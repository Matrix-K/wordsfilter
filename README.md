# wordsfilter
a extension for php to filter some keywords and senstive words

# Install 
1.Download the repo from master branch
2.Install libdatrie library
3.Install the extensions

# How to Use

```php
<?php
ini_set('display_errors', 1);
$arrWord = array('word', 'word1', 'word3', 'a', 'ab', 'abc', 'b');
$resTrie = wordsfilter_create_instance(); //create an empty trie tree

foreach ($arrWord as $k => $v) {
        wordsfilter_create_word($resTrie, $v);
}
wordsfilter_save($resTrie, __DIR__ . '/blackword.tree');
$resTrie = wordsfilter_load(__DIR__ . '/blackword.tree');
$str='hello word2 haha word1 word4 word2';
$arrRet = wordsfilter_search($resTrie, $str);
print_all($str,array($arrRet)); //Array(0 => 6, 1 => 5)
echo "\ntest1///////////////////\n";
$str = 'hello word2 haha word1 word4 word2';
$arrRet = wordsfilter_search_all($resTrie, $str);
print_all($str, $arrRet);
echo "\ntest2///////////////////\n";
$str = 'hello word';
$arrRet = wordsfilter_search($resTrie, $str);
print_all($str, array($arrRet)); //Array()
$arrRet = wordsfilter_search_all($resTrie, 'hello word');
print_all($str, $arrRet);
echo "\ntest3///////////////////\n";
echo "start memory=".memory_get_usage(true)."\n";date_default_timezone_set('Asia/Shanghai');
$test = array('a', 'abd', 'dad', 'pab', 'dda', 'word1f', 'cword1', 'cword1t');
foreach ($test as $v) 
{
    $arrRet = wordsfilter_search_all($resTrie, $v);
}
echo "end memory=".memory_get_usage(true)."\n";
echo date('Y-m-d H:i:s');
wordsfilter_free($resTrie);
function print_all($str, $res) {//print_r($res);
    echo "$str\n";
    foreach ($res as $k => $v) {
        echo $k."=>{$v[0]}-{$v[1]}-".substr($str, $v[0], $v[1])."\n";
    }
}



```
