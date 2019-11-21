<?php
require_once("deploy.php");

$cfe = new CfeRamStream();
$cfe->stream_open("cfe:///dev/ttyUSB0", "r", null, $dummy);

$nops = array(
	0x00F016FC, //don't disable IRQ/FIQ disable
	0x00F0170C, //don't disable DCache
	0x00F01710, //don't disable MMU
	0x00F01714  //don't disable ICache
);
foreach($nops as $addr){
	$cfe->cfe_sm("\x00\x00\x00\x00", 4, $addr);
}