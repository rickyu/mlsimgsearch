struct SpatialRankerRequest {
       1: string keypoints;
       2: string descriptors;
       3: list<i64> pics;
}

struct SpatialSimilar {
       1: i64 picid;
       2: double sim;
}

struct SpatialRankerResponse {
       1: list<SpatialSimilar> result;
}

service SpatialRankerMsg {
	SpatialRankerResponse Rank(1:SpatialRankerRequest request);
}
