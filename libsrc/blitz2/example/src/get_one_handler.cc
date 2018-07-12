#include <iostream>
#include "get_one_handler.h"

using namespace blitz2;

int GetOneHandler::Process(::apache::thrift::protocol::TProtocol* iprot, 
                      int seqid, ThriftServer::TSharedChannel* channel) {
  LOG_DEBUG(logger_, "GotRequest seqid=%d fd=%d", seqid, channel->fd());
  Test_GetOne_args args;
  args.read(iprot);
  iprot->readMessageEnd();
  uint32_t bytes = iprot->getTransport()->readEnd();
  (void)(bytes);
  Context* context = new Context(logger_, args.key, redis_pool_);
  state_machine_.StartOneContext(context, 
      std::bind(OnStateMachineEnd, std::placeholders::_1, seqid, 
        SharedPtr<ThriftServer::TSharedChannel>(channel)));
  return 0;

}
int GetOneHandler::OnStateMachineEnd(Context* context, int seqid,
    SharedPtr<ThriftServer::TSharedChannel> channel) {
  Test_GetOne_result result;
  result.success.__set_result_(context->get_result());
  result.success.__set_key_(context->get_key());
  result.success.__set_value_(context->get_value());
  result.__isset.success = true;
  SharedPtr<OutMsg> outmsg = blitz2::CreateOutmsgFromThriftStruct(
      result, "GetOne", ::apache::thrift::protocol::T_REPLY, seqid);
  channel->SendMsg(outmsg.get_pointer());
  return 0;
}
void GetOneHandler::InitStateMachine() {
  StateMachine::State states[] = {
         {kStateEnter, &Context::OnEnter, ActionArcs()(0, kStateWaitRedis)},
         {kStateWaitRedis, NULL, ActionArcs()},
         {kStateEnd, &Context::OnEnd, ActionArcs()}
                                  };
  StateMachine::Transition trans[] = {
         {kStateWaitRedis, kEventRedisResp, &Context::OnWaitRedisMeetRedisResp, ActionArcs()(0,0)(1,kStateEnd)}
                                  };

   int ret =  state_machine_.Init(states, sizeof(states)/sizeof(StateMachine::State),
                      trans, sizeof(trans)/sizeof(StateMachine::Transition),
                      kStateEnter, kStateEnd);
   if (ret != 0) {
     abort();
   }

}

action_result_t GetOneHandler::Context::OnEnter(uint32_t context_id, StateMachine* machine) {
  LOG_DEBUG(logger_, "StateStart::Enter context=%d", context_id );
   char *buf = new char[key_.size() + 3];
   redis_req_count_ = 3;
   redis_reply_.resize(3);
   sprintf(buf, "%s:1", key_.c_str());
   TRedisEventFire redis_fire(redis_pool_, kEventRedisResp);
   redis_fire.FireGet(buf, context_id, machine, reinterpret_cast<void*>(0), 200,
       LogId(context_id, 0));
   redis_reply_[0].first = buf;

   sprintf(buf, "%s:2", key_.c_str());
   redis_fire.FireGet(buf, context_id, machine, reinterpret_cast<void*>(1), 200,
       LogId(context_id, 1));
   redis_reply_[1].first = buf;
   sprintf(buf, "%s:3", key_.c_str());
   redis_fire.FireGet(buf, context_id, machine, reinterpret_cast<void*>(2), 200,
       LogId(context_id, 2));
   redis_reply_[2].first = buf;
   delete [] buf;
   return 0;
}
 action_result_t GetOneHandler::Context::OnWaitRedisMeetRedisResp(
                        Event* event,
                        uint32_t context_id,
                        StateMachine* machine) {
   (void)(machine);
   assert(event->type() == kEventRedisResp);
   ++redis_resp_count_;
   RedisEvent* event_data = dynamic_cast<RedisEvent*>(event);
   int req_no = static_cast<int>(reinterpret_cast<int64_t>(event_data->param()));
   LOG_DEBUG(logger_, "redisReply no=%d context=%d", req_no, context_id);
   if (req_no >= 3 || req_no < 0) {
     printf("error");
     return 0;
   }
   if (event_data->reply()) {
     redisReply* reply = event_data->reply()->get_reply();
     if (reply->type == REDIS_REPLY_STRING) {
       LOG_DEBUG(logger_, "redisReply type=string no=%d context=%d",
           req_no, context_id);
       redis_reply_[req_no].second.assign(reply->str, reply->len);
     } else {
       LOG_ERROR(logger_, "redisReply type=string no=%d context=%d",
           req_no, context_id);
     }
   } 
   if (redis_resp_count_ == redis_req_count_) {
     return 1;
   } else {
     return 0;
   }
 }
 action_result_t GetOneHandler::Context::OnEnd(uint32_t context_id, StateMachine* machine) {
   (void)(machine);
   LOG_DEBUG(logger_, "context_id=%u", context_id);
   value_.clear();
   for (int i=0; i<static_cast<int>(redis_reply_.size()); ++i) {
      if (i>0) {
        value_ += ',';
      }
      value_ += redis_reply_[i].first + ':' + redis_reply_[i].second;
    }
   return 0;
 }

