
struct TBOFQuantizeResult {
       1: double q;
       2: list<string> word_info;
}

struct ExecMergeRequest {
       1: map<i64,TBOFQuantizeResult> img_qdata;
}

struct ExecMergeResponse {
       1: i32 ret_num;
       2: binary result;
       3: i32 result_len;
       4: double query_pow;
}

service ExecMergeMsg {
	ExecMergeResponse Merge(1:ExecMergeRequest request);
}
