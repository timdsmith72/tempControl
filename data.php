<?php

$dayz = time();
$aa = (($_GET['Temperature']));
$ab = (($_GET['Setpoint']));
$ac = (($_GET['Input']));
$ag = (($_GET['Heater']));
$db="tempMonitor";
$link = mysql_connect('', 'username', 'password');
if (! $link) die(mysql_error());
mysql_select_db($db , $link) or die("Couldn't open $db: ".mysql_error());

$queryResult = mysql_query("INSERT INTO temp (Time, Temp, Setpoint, Input, Heat) VALUES ('$dayz', '$aa', '$ab', '$ac', '$ag')");

?>