<?php
class CfeRamStream {
	private $tty;
	private $addr = 0;

	private $pos;

	function runCmd($cmd){
		fwrite($this->tty, $cmd);
		fflush($this->tty);
		$ack = fgets($this->tty);
	}

	function stream_open($path, $mode, $options, &$opened_path){
		// dirty for now
		$dev = preg_replace("/^cfe:\/\//", "", $path);

		//print("opening {$dev}\n");
		// assume $path to be a TTY
		system("stty -F {$dev} ispeed 115200 ospeed 115200 cs8 ixoff");
		$this->tty = fopen($dev, "r+");
		return is_resource($this->tty);
	}

	function stream_set_option($option, $arg1, $arg2){
		switch($option){
			case STREAM_OPTION_READ_BUFFER:
				stream_set_read_buffer($this->tty, $arg1);
				break;
			case STREAM_OPTION_WRITE_BUFFER:
				stream_set_write_buffer($this->tty, $arg1);
				break;
		}
	}

	function isAtPrompt(){
		fflush($this->tty);

		stream_set_blocking($this->tty, false);
		// get all available content
		$data = stream_get_contents($this->tty);
		stream_set_blocking($this->tty, true);

		if(empty($data)){
			//tty is silent, write newline and retry
			fwrite($this->tty, "\r\n");
			fflush($this->tty);
			return $this->isAtPrompt();
		}
		
		// buffer must end with "CFE>"
		$trailing = strrchr($data, "CFE> ");
		return $trailing == "CFE> ";

		/*$lines = preg_split("/\r?\n/", $data);
		$lastLine = end($lines);
		return (strpos($lastLine, "CFE>") === 0);*/
			
	}

	function writeCommand($cmd){
		fflush($this->tty);

		stream_set_blocking($this->tty, false);

		do {
			// get all available content
			$data = trim(stream_get_contents($this->tty));
			$trailing = strrchr($data, "CFE>");
		} while(!empty($data));
		stream_set_blocking($this->tty, true);


		// "CFE>" was at the end of the buffer or tty is silent
		if($trailing == "CFE>" || $trailing === FALSE){
			//tty is silent, write command
			fwrite($this->tty, $cmd);
			fflush($this->tty);
		}
	}

	function ensureCfePrompt(){
		while(!$this->isAtPrompt()){
			//print("wait\n");
			usleep(10000);
		}
		/*if(!$this->isAtPrompt()){
			fwrite($this->tty, "\r\n");
			$ack = fgets($this->tty);
		}*/
		/*while(true){
			$ack = fgets($this->tty);
			var_dump($ack);
			if($ack === FALSE){
				fwrite($this->tty, "\r\n");
				continue;
			}

			if(strpos($ack, "CFE>") === 0)
				return;

			continue;
		}*/
	}

	function stream_close(){
		fclose($this->tty);
	}

	function stream_read($count){
		$this->ensureCfePrompt();

		$cmd = sprintf("db 0x%x 0x%x\r\n", $this->addr, $count);
		fwrite($this->tty, $cmd);
		fflush($this->tty);
		$ack = fgets($this->tty);

		$data = "";
		while(true){
			$line = fgets($this->tty);
			if(strpos($line, "CFE>") === 0)
				break;

			$line = preg_replace("/\r?\n/", "", $line);
		    if(empty($line))
		        continue;

		    if(!preg_match("/([0-9a-fA-F])+: (.*)/", $line, $m))
		        continue;

		    $hex = $m[2];
		    $hex  = substr($hex, 0, strlen($hex) - 17);
		    $hex = str_replace(' ', '', $hex);
		    $bin = hex2bin($hex);
			$data .= $bin;
		}

		return $data;
	}

	// TODO: do this on binary, not hex
	function hexEndianSwap($hex){
		return implode('', array_reverse(str_split($hex, 2)));
	}

	function cfe_sm($data, $size, $addr = null){
		$hex = $this->hexEndianSwap(bin2hex($data));

		$theAddr = (is_null($addr)) ? $this->addr : $addr;

		$cmd = sprintf("sm 0x%x 0x%s {$size}\r\n", $theAddr, $hex);
		print($cmd);
		
		//$this->ensureCfePrompt();
		fwrite($this->tty, $cmd);
		
		while(true){
			$ack = fgets($this->tty);
			//var_dump($ack);
			if(strpos($ack, '***') !== 0)
				continue;

			$ack = rtrim($ack);
			$code = substr($ack, -1);
			if($code !== '0'){
				fwrite(STDERR, $ack . PHP_EOL);
				die("cfe_sm: failure" . PHP_EOL);
			}
			break;
		}

		fgets($this->tty); //this line HAS to be the prompt after the *** line
		//var_dump($ack);


		//$this->writeCommand($cmd);


		fflush($this->tty);
		
		$this->addr += $size;
	}

	private $buffer;

	function stream_flush(){
		$data = $this->buffer;
		$length = strlen($data);

		$numWords = intval($length / 4);
		// todo: split into HalfWords too
		$numBytes = $length % 4;

		$offset = 0;
		for($i=0; $i<$numWords; $i++, $offset += 4){
			$dw = substr($data, $offset, 4);
			$this->cfe_sm($dw, 4);
		}
		for($i=0; $i<$numBytes; $i++, $offset++){
			$byte = substr($data, $offset, 1);
			$this->cfe_sm($byte, 1);
		}

		// the gap with the buffer should be 0 now
		assert($this->addr == $this->pos);

		$this->buffer = "";
	}

	function stream_write($data){
		$length = strlen($data);
		$this->buffer .= $data;
		$this->pos += $length;
		return $length;
	}

	function stream_tell(){
		return $this->pos;
	}

	function stream_eof(){
		return false;
	}

	function stream_seek($offset, $whence){
		switch($whence){
			case SEEK_SET:
				$this->pos = $offset;
				break;
			case SEEK_CUR:
				$this->pos += $offset;
				break;
			case SEEK_END:
				return false;
		}
		$this->addr = $this->pos;
	}
}

stream_wrapper_register("cfe", "CfeRamStream") or die("fail" . PHP_EOL);

$cfe = fopen("cfe:///dev/ttyUSB0", "r");

/*
$length = 0x50;
$someShit = openssl_random_pseudo_bytes($length);
file_put_contents("src.bin", $someShit);

$sourceHash = sha1($someShit);


fseek($cfe, (int)0x0);

fwrite($cfe, $someShit);
fflush($cfe);
*/

if (basename(__FILE__) != basename($_SERVER["SCRIPT_FILENAME"])) {
	return;
}

if($argc < 3){
	fwrite(STDERR, "Usage: {$argv[0]} <filename> <0xaddress>" . PHP_EOL);
	return 1;
}

$filepath = $argv[1];
$target_addr = $argv[2];


$payload = fopen($filepath, "rb");

$offset = hexdec($target_addr);
fseek($cfe, (int)$offset);
print("=> write" . PHP_EOL);
stream_copy_to_stream($payload, $cfe);
fflush($cfe);

$length = ftell($payload);
fclose($payload);


/*
$sourceHash = sha1_file($filepath);

fseek($cfe, (int)$offset);
print("=> read" . PHP_EOL);
$readBack = fread($cfe, $length);

$destHash = sha1($readBack);
print("{$sourceHash} => {$destHash}" . PHP_EOL);
*/

/*file_put_contents("rb.bin", $readBack);*/

fclose($cfe);

/*
print("Executing program @{$target_addr}" . PHP_EOL);
$tty = fopen("/dev/ttyUSB0", "r+");
fwrite($tty, "\r\ngo {$target_addr}\r\n");
fflush($tty);
fclose($tty);
*/
