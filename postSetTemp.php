<?php

$setTemp = check_input($_GET['setTemp'], "Enter a Temperature");
//if (preg_match("/\D/",$setTemp))
$valid = checkIfValid($setTemp);
if (! $valid)
{
   die("Please enter numbers only for Temperature");
}
$db = "tempMonitor";
$link = mysql_connect('', 'username', 'password');
if (! $link) die(mysql_error());
mysql_select_db($db , $link) or die("Couldn't open $db: ".mysql_error());
# $queryResult = mysql_query("UPDATE SetPoint (SetTemp) VALUES ('$setTemp')");

if ($queryResult = mysql_query("UPDATE SetPoint SET SetTemp = '$setTemp'"))
{
   mysql_close($link);
   echo "Database update successful!!\n\r";
   echo "Redirecting back in 5 seconds.";
   $page = "dashboard.php";
   header("Refresh: 5; URL=\"" . $page . "\"");
}
?>

<?php
function checkIfValid($setTemp) {
    $regex = '/^\s*[+\-]?(?:\d+(?:\.\d*)?|\.\d+)\s*$/';
    return preg_match($regex, $setTemp); 
}
?>

<?php
function check_input($data, $problem='')
{
    $data = trim($data);
    $data = stripslashes($data);
    $data = htmlspecialchars($data);
    if ($problem && strlen($data) == 0)
    {
        die($problem);
    }
    return $data;
}
?>
