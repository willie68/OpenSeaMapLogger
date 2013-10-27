<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
	"http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
	<meta http-equiv="Content-Language" content="en" />
	<title>Open Sea Map Logger Configuration</title>
</head>
<body>
<h1>Open Sea Map Logger Configuration</h1>
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
</table>
	<br/>
	<input type="submit" class="submit button" name="add" value="Create" />
	After pressing [create], save the file (config.dat) into the root folder of the logger sd card.
</form>
</body>
</html>