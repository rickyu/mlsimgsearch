<?php

$file_handle = file($argv[1]);
$imgsearch_url_prefix = "http://124.202.144.15/search/common?user=yantao&url=http://124.202.144.15/";
foreach ($file_handle as $line) {
    printf("begin process line:%s\n", $line);
    $fields = explode("\t", $line);
    $similars = explode(",", $fields[1]);
    $img_key = $fields[0];
    $url = $imgsearch_url_prefix  . $img_key;
    $ch = curl_init($url);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
    curl_setopt($ch, CURLOPT_AUTOREFERER,TRUE);
    $bytes = curl_exec($ch);
    $curl_errno = curl_errno($ch);
    $curl_error = curl_error($ch);
    $code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    curl_close($ch);
    $parts = json_decode($bytes, true);
    $precision_items_num = 0;
    $result_pics = "";
    $src_pics = "";
    foreach ($parts['data']['same'] AS $item) {
        $result_pics .= $item['picid'];
        $result_pics .= ",";
    }
    foreach ($similars AS $sim_item) {
        $src_pics .= $sim_item;
        $src_pics .= ",";
    }
    foreach ($parts['data']['same'] AS $item) {
        foreach ($similars AS $sim_item) {
            if ($item['picid'] == $sim_item) {
                ++$precision_items_num;
                break;
            }
        }
    }
    $callback_items_num = 0;
    foreach ($similars AS $sim_item) {
	    foreach ($parts['data']['same'] AS $item) {
	        if ($item['picid'] == $sim_item) {
		        ++$callback_items_num;
		        break;
	        }   
	    }
    }
    echo "result_pics=$result_pics, src_pics=$src_pics ". $img_key . ": precision_rate => " . $precision_items_num / count($parts['data']['same'])
	. "; callback_rate => " . $callback_items_num / count($similars) . "\n";
}
