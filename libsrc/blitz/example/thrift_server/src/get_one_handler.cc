#include <iostream>
#include "get_one_handler.h"

int GetOneHandler::Process(::apache::thrift::protocol::TProtocol* iprot, 
                      int seqid, const blitz::IoInfo& ioinfo) {
  ContextEndParam* my_param = new ContextEndParam();
  my_param->self = this;
  my_param->io_id = ioinfo.get_id();
  my_param->seqid = seqid;
  Test_GetOne_args args;
  args.read(iprot);
  iprot->readMessageEnd();
  uint32_t bytes = iprot->getTransport()->readEnd();
  (void)(bytes);
  GetOneHandler::Input input;
  input.key = args.key;
  input.redis_pool = redis_pool_;
  state_machine_.StartOneContext(
      input, OnStateMachineEnd, my_param);
  return 0;

}
int GetOneHandler::OnStateMachineEnd(int res,
                                     const GetOneHandler::Output* output,
                                     void* user_params) {
  ContextEndParam* param = 
                reinterpret_cast<ContextEndParam*>(user_params);
  Test_GetOne_result result;
  result.success.__set_result_(output->result);
  result.success.__set_key_(output->key);
  result.success.__set_value_(output->value);
  result.__isset.success = true;
  blitz::ThriftServer::SendReply(&param->self->framework_, param->io_id,
                               result,
                               "GetOne",
                               ::apache::thrift::protocol::T_REPLY,
                               param->seqid);
  delete param;
  return 0;
}
void GetOneHandler::InitStateMachine() {
  StateMachine::State states[] = {
         {kStateEnter, &Context::OnEnter},
         {kStateWaitRedis, NULL},
         {kStateEnd, &Context::OnEnd}
                                  };
  StateMachine::Transition trans[] = {
         {kStateWaitRedis, kEventRedisResp, &Context::OnWaitRedisMeetRedisResp}
                                  };

   int ret =  state_machine_.Init(states, sizeof(states)/sizeof(StateMachine::State),
                      trans, sizeof(trans)/sizeof(StateMachine::Transition),
                      kStateEnter, kStateEnd);
   if (ret != 0) {
     printf("fatal:Init GetOne StateMachine\n");
     abort();
   }

}

int GetOneHandler::Context::OnEnter(int context_id, StateMachine* machine) {
   std::cout<<"StateStart::Enter" << std::endl;
   char *buf = new char[key_.size() + 3];
   redis_req_count_ = 3;
   redis_reply_.resize(3);
   sprintf(buf, "%s:1", key_.c_str());
   printf("get %s\n", buf);
   blitz::RedisEventFire<StateMachine> redis_fire(redis_pool_, kEventRedisResp);
   redis_fire.FireGet(buf, context_id, machine, reinterpret_cast<void*>(0), 200);
   redis_reply_[0].first = buf;

   sprintf(buf, "%s:2", key_.c_str());
   redis_fire.FireGet(buf, context_id, machine, reinterpret_cast<void*>(1), 200);
   redis_reply_[1].first = buf;
   sprintf(buf, "%s:3", key_.c_str());
   redis_fire.FireGet(buf, context_id, machine, reinterpret_cast<void*>(2), 200);
   redis_reply_[2].first = buf;
   return kStateWaitRedis;
}
 int GetOneHandler::Context::OnWaitRedisMeetRedisResp(
                        blitz::Event* event,
                        int context_id,
                        StateMachine* machine) {
   assert(event->type == kEventRedisResp);
   ++redis_resp_count_;
   blitz::RedisEventData* event_data = reinterpret_cast<blitz::RedisEventData*>(event->data);
   int req_no = static_cast<int>(reinterpret_cast<int64_t>(event_data->param));
   if (req_no >= 3 || req_no < 0) {
     printf("error");
     return 0;
   }
   std::cout<<"StateWaitRedis::GetRedisReply:" << req_no << std::endl;
   if (event_data->reply) {
     redisReply* reply = event_data->reply;
     if (reply->type == REDIS_REPLY_STRING) {
       redis_reply_[req_no].second.assign(reply->str, reply->len);
     } 
     freeReplyObject(event_data->reply);
   } 
   if (redis_resp_count_ == redis_req_count_) {
     return kStateEnd;
   } else {
     return 0;
   }
 }
 int GetOneHandler::Context::OnEnd(int context_id, StateMachine* machine) {
   std::cout<<"StateEnd::Enter" << std::endl;
   return 0;
 }

