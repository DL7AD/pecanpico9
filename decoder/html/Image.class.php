<?php
class Image {

	function __construct($sqlResult) {

		$this->id = $sqlResult['id'];
		$this->call = $sqlResult['call'];

		$this->time_first = $sqlResult['time_first'];
		$this->time_last = $sqlResult['time_last'];

		$this->imageID = $sqlResult['imageID'];
		$this->packetID = $sqlResult['packetID'];
		$this->count = $sqlResult['count'];

	}
}
?>

