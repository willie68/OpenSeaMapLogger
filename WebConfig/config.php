<?php
  header('Content-type: text/plain');

  // Es wird downloaded.pdf benannt
  header('Content-Disposition: attachment; filename="config.dat"');

 $baud_a = $_POST["baud_a"];
 $seatalk = $_POST["seatalk"];
 $baud_b = $_POST["baud_b"];
 if ($seatalk) {
   $baud_a = "3";
 }
 echo "$seatalk$baud_a\r\n";
 echo "$baud_b\r\n";
?>
