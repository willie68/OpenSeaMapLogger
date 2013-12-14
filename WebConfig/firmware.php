<?php
  header('Content-type: application/octet-stream');

  // Es wird downloaded.pdf benannt
  header('Content-Disposition: attachment; filename="OSMFIRMW.HEX"');
  readfile('OSMFIRMW.HEX');
?>
