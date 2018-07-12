
struct GetResp {
    1: i32 result_,
    2: string key_,
    3: string value_,
}


service Test {
  GetResp GetOne(1:string key),
}

