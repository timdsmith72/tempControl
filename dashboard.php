<html>
<head>
<meta http-equiv="refresh" content="30">
<script type="text/javascript" src="https://www.google.com/jsapi"></script>
<script type="text/javascript">
google.load("visualization", "1", {packages:["corechart"]});
google.setOnLoadCallback(drawChart);

function drawChart() {
   var data = new google.visualization.DataTable();
   data.addColumn('string', 'Time');
   data.addColumn('number', 'Temp');
   //data.addColumn('number', 'Input');
   data.addColumn('number', 'Setpoint');

   data.addRows([
   <?php

   $db="tempMonitor";
   $link = mysql_connect('', 'username', 'password');
   mysql_query('SET NAMES utf8'); 
   mysql_select_db($db , $link) or die("Couldn't open $db: ".mysql_error());
   // Retrieve all the data from the "temp" table
   $result = mysql_query("SELECT TIMESTAMPDIFF(MINUTE,FROM_UNIXTIME(Time), now()) as Time1, Temp, Setpoint FROM temp WHERE TIMESTAMPDIFF(MINUTE,FROM_UNIXTIME(Time), now()) < 160 Order By Time");
   if ($result !== false) {
      $num=mysql_numrows($result);
      $i=0;
      echo"";

      while ($i < $num) {
         $time=mysql_result($result,$i,"Time1");
         $temp=mysql_result($result,$i,"Temp");
         //$input=mysql_result($result,$i,"Input");
         $setpoint=mysql_result($result,$i,"Setpoint");
         echo "['";
         echo "$time";
         echo "',";
         echo "$temp";
         echo ",";
         echo "$setpoint";
         echo "]";
         if ($i < ($num - 1)) 
         {
            echo ",";
         }
         $i++;
      }
   }

   ?> 
   ]);

   var options = {
      width: 1000, height: 300,
      hAxis: {title: 'Minutes Ago'},
      vAxis: {title: 'Temperature F', maxValue: 88, minValue: 45}
   };

   var chart = new google.visualization.LineChart(document.getElementById('chart_div1'));
   chart.draw(data, options);
}
</script>

<script type='text/javascript' src='https://www.google.com/jsapi'></script>
<script type='text/javascript'>
google.load('visualization', '1', {packages:['gauge']});
google.setOnLoadCallback(drawChart);
function drawChart() {
   var data = new google.visualization.DataTable();
   data.addColumn('string', 'Label');
   data.addColumn('number', 'Value');
   data.addRows([
   <?php

   $db="tempMonitor";
   $link = mysql_connect('', 'username', 'password');
   mysql_query('SET NAMES utf8'); 
   mysql_select_db($db , $link) or die("Couldn't open $db: ".mysql_error());
   // Retrieve all the data from the "temp" table
   $result = mysql_query("SELECT Time, Temp, Setpoint FROM temp Order By Time desc limit 1");
   mysql_close($link);
   if ($result !== false) {
      $num=mysql_numrows($result);
      $time=mysql_result($result,0,"Time");
      $temp=mysql_result($result,0,"Temp");
      $setpoint=mysql_result($result,0,"Setpoint");
      $link2 = mysql_connect('', 'username', 'password');
      mysql_query('SET NAMES utf8'); 
      mysql_select_db($db , $link2) or die("Couldn't open $db: ".mysql_error());
      $result2 = mysql_query("SELECT SetTemp FROM SetPoint");
      mysql_close($link2);
      if ($result !== false) {
         $setTemp=mysql_result($result2,0,"SetTemp");
      }
      echo "['Temp',";
      echo "$temp";
      echo "],";
      echo "['Target Temp',";
      echo "$setTemp";
      echo "]";
   }

   ?> 
   ]);

   var options2 = {
      width: 400, height: 150,
      minorTicks: 2, max: 212, min: 60
   };

   var chart = new google.visualization.Gauge(document.getElementById('chart_div2'));
   chart.draw(data, options2);
}
</script>

<script type='text/javascript' src='https://www.google.com/jsapi'></script>
<script type='text/javascript'>
google.load('visualization', '1', {packages:['gauge']});
google.setOnLoadCallback(drawChart);

function drawChart() {
   var data = new google.visualization.DataTable();
   data.addColumn('string', 'Label');
   data.addColumn('number', 'Value');
   data.addRows([
   <?php
   $db="tempMonitor";
   $link = mysql_connect('', 'username', 'password');
   mysql_query('SET NAMES utf8'); 
   mysql_select_db($db , $link) or die("Couldn't open $db: ".mysql_error());
   // Retrieve all the data from the "temp" table
   $result = mysql_query("SELECT SetTemp FROM SetPoint");
   if ($result !== false) {
      $num=mysql_numrows($result);
      $heat=mysql_result($result,0,"SetTemp");

      echo "['SetTemp %',";
      echo "$SetTemp";
      echo "]";
   }
   ?> 
   ]);

   var options3 = {
   width: 200, height: 300,
   minorTicks: 2, max: 100, min: 0
   };

   var chart = new google.visualization.Gauge(document.getElementById('chart_div3'));
   chart.draw(data, options3);
}
</script>


</head>
   <body>
   <div align="center">
      <H1 align="center">Your page Heading</H1>
      <form action="postSetTemp.php" method="GET">
      <p align="center">Set Temp:  <input type="text" name="setTemp" /><br /></p>
      <p align="center"><input type="submit" value="Set"></p>
      </form>
   </div>
<div align="center">
<table width="900" border="1">
   <tr>
      <td><div align="center">
         <table width="1000" border="0">
            <tr>
               <td width="900"><div id="chart_div1"></div></td>
            </tr>
         </table>
         </div>
      </td>
   </tr>
   <tr>
      <td><div align="center">
         <table width="900" border="0">
            <tr>
               <td width="400"><div id="chart_div2" align="center"></div></td>
            </tr>
         </table>
         </div>
      </td>
   </tr>
</table>
</div>
</body>
</html>
