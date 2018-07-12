<?php

/**
 * 分析搜索结果
 * @param type: 1. 得到相同记过，2. 得到相似结果
 * @return: 返回pic_id，以空格分隔
 */
if ($argc < 2) {
    echo "Usage: php parse_json_result.php type";
    exit(1);
}

$type = $argv[1];
$file = STDIN;
if (feof($file)) {
    exit(2);
}

$content = fgets($file);
$json_content = json_decode($content, TRUE);
$result = "";

switch ($type) {
case 1:
    for ($i = 0; $i < count($json_content['data']['same']); ++$i) {
	$result .= $json_content['data']['same'][$i]['picid'];
	$result .= " ";
    }
    break;
case 2:
    for ($i = 0; $i < count($json_content['data']['similar']); ++$i) {
	$result .= $json_content['data']['similar'][$i]['picid'];
	$result .= " ";
    }
    break;
default:
    exit(3);
}

echo $result;

exit(0);