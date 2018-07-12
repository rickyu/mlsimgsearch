struct ImageSearchRequest {
       1: i32 type;
       2: string key;
       3: string workload;
}

struct ImageSearchResponse {
	1: i32 return_code;
	2: binary img_data;
	3: i32 img_data_len;
}

service ImageSearchMsg {
	ImageSearchResponse Search(1:ImageSearchRequest request);
}
