<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
	"http://www.w3.org/TR/html4/loose.dtd">
<?php
include ("config/config.php");

// connect to the database
$conn = mysql_connect($db_host, $db_user, $db_pass) or die("Could not connect to database!");
mysql_select_db($db, $conn);


	$downloadID = 61;
	$eventQuery = "SELECT * FROM download WHERE (ID = '$downloadID');";
	$eventExec = mysql_query($eventQuery);
	while($row = mysql_fetch_array($eventExec)) {
		$filename = $row["filename"];
		$count = $row["count"];
		$version = $row["Version"];
	}
?>
<html>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
	<meta http-equiv="Content-Language" content="en" />
	<title>Open Sea Map Logger Configuration</title>
</head>
<body>
<h1>Open Sea Map Logger Configuration</h1>

Here you can download the actual firmware file: <a href="http://wkla.dyndns.org/downloader/download.php?ID=61">FIRMWARE <?php echo "$version"; ?></a><br>
<a href="readme.html" target="_blank">readme</a> and <a href="OSM_DL_Anleitung.pdf" target="_blank">Manual</a><br>
The source code is developt under LGPL License and can be found here: <a href="https://github.com/OpenSeaMap/logger-oseam-0183" target="_blank">Github repository</a>
<br/>
<hr/>
<h2>Configuration</h2>
<form name="config" action="config.php" method="post">
<table>
	<tr>
		<td valign="top"><b>NMEA A</b></td>
		<td valign="top">Seatalk:</td>
		<td><input type="checkbox" name="seatalk" value="s" onchange="forms.config.baud_a.disabled = forms.config.seatalk.checked;" ></td>
		<td valign="top">If you select seatalk, please make sure to set the internal jumper in the logger to thr seatalk position.</td>
	</tr>
	
	<tr>
		<td valign="top"></td>
		<td valign="top">Baudrate:</td>
		<td><select name="baud_a" size="1">
				<option value="0">inactive</option>
				<option value="1">1200</option>
				<option value="2">2400</option>
				<option value="3" selected>4800 (*)</option>
				<option value="4">9600</option>
				<option value="5">19200</option>
			</select>
		</td>
		<td valign="top">(*) Default.</td>
	</tr>
	<tr>
		<td valign="top"><b>NMEA B</b></td>
		<td valign="top">Baudrate:</td>
		<td><select name="baud_b" size="1">
				<option value="0">inactive</option>
				<option value="1">1200</option>
				<option value="2">2400</option>
				<option value="3" selected>4800 (*)</option>
			</select>
		</td>
		<td valign="top">(*) Default. The NMEA B connector couldn't be configured beyound 4800 baud.</td>
	</tr>
	<tr>
		<td valign="top"><b>Features</b></td>
		<td valign="top">&nbsp;</td>
		<td>
		  <input type="checkbox" name="outputGyro" value="2" checked="checked"/>write Gyrodata * (1)<br>
		  <input type="checkbox" name="outputVcc" value="1"/>write board supply (2)
		</td>
		<td valign="top">(*) Default. Here you can de/activate special logger features.</td>
	</tr>
</table>
	<br/>
	<input type="submit" class="submit button" name="add" value="Create" />
	After pressing [create], save the file (config.dat) into the root folder of the logger sd card.
</form>
<br>
(1) you can deactivate the writing of gyro data. This saves space on the sd card, but the data may be useless.<br>
(2) you can activate the writing of special NMEA Messages for the board supply. Should be activated only in case of a support request.<br>


<div id="footer">
	<p align="center"><a href="http://www.disclaimer.de/disclaimer.htm?farbe=FFFFFF/000000/000000/000000">Haftungsausschluss</a></p>
	<p align="center"><a href="http://www.wk-musik.tk">Willies world</a></p>
</div>

</body>
</html>