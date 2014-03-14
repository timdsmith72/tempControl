<?php
$db="tempMonitor";
$link = mysql_connect('', 'username', 'password');
mysql_query('SET NAMES utf8');
mysql_select_db($db , $link) or die("Couldn't open $db: ".mysql_error());
$result = mysql_query("SELECT SetTemp FROM SetPoint");
mysql_close($link);
if ($result !== false) {
   $setTemp=mysql_result($result,0,"SetTemp");
}
echo '<'.$setTemp.'>';

?>