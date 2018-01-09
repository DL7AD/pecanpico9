<?php
class Telemetry {

	/*const TYPE_INT		= 0; // Integer
	const TYPE_STR		= 1; // String
	const TYPE_HTML		= 2; // HTML

	const LOC_INT		= 0; // Internal
	const LOC_EXT		= 1; // External

	const LOC_INT1		= 0; // Internal 1
	const LOC_EXT1		= 1; // External 1
	const LOC_EXT2		= 2; // External 2

	const COL_GREEN		= "#008000";
	const COL_ORANGE	= "#CC6600";
	const COL_RED		= "#FF0000";*/

	function __construct($sqlResult) {
		$this->reset = $sqlResult['reset'];
		$this->id = $sqlResult['id'];

		$this->rxtime = $sqlResult['rxtime'];

		$this->call = $sqlResult['call'];

		$this->adc_vsol = $sqlResult['adc_vsol'];
		$this->adc_vbat = $sqlResult['adc_vbat'];
		$this->pac_vsol = $sqlResult['pac_vsol'];
		$this->pac_vbat = $sqlResult['pac_vbat'];
		$this->pac_pbat = $sqlResult['pac_pbat'];
		$this->pac_psol = $sqlResult['pac_psol'];

		$this->gps_time = $sqlResult['gps_time'];
		$this->gps_lock = $sqlResult['gps_lock'];
		$this->gps_sats = $sqlResult['gps_sats'];
		$this->gps_ttff = $sqlResult['gps_ttff'];
		$this->gps_pdop = $sqlResult['gps_pdop'];
		$this->gps_alt = $sqlResult['gps_alt'];
		$this->gps_lat = $sqlResult['gps_lat'];
		$this->gps_lon = $sqlResult['gps_lon'];

		$this->sen_i1_press = $sqlResult['sen_i1_press'];
		$this->sen_e1_press = $sqlResult['sen_e1_press'];
		$this->sen_e2_press = $sqlResult['sen_e2_press'];
		$this->sen_i1_temp = $sqlResult['sen_i1_temp'];
		$this->sen_e1_temp = $sqlResult['sen_e1_temp'];
		$this->sen_e2_temp = $sqlResult['sen_e2_temp'];
		$this->sen_i1_hum = $sqlResult['sen_i1_hum'];
		$this->sen_e1_hum = $sqlResult['sen_e1_hum'];
		$this->sen_e2_hum = $sqlResult['sen_e2_hum'];

		$this->stm32_temp = $sqlResult['stm32_temp'];
		$this->si4464_temp = $sqlResult['si4464_temp'];

		$this->light_intensity = $sqlResult['light_intensity'];

		$this->sys_time = $sqlResult['sys_time'];
		$this->sys_error = $sqlResult['sys_error'];

		$this->err_i2c1 = ($this->sys_error >> 0) & 0x1;
		$this->err_i2c2 = ($this->sys_error >> 1) & 0x1;
		$this->err_eva7m = ($this->sys_error >> 2) & 0x1;
		$this->err_pac1720 = ($this->sys_error >> 3) & 0x3;
		$this->err_ov5640 = ($this->sys_error >> 5) & 0x3;
		$this->err_bme280_i1 = ($this->sys_error >> 8) & 0x1;
		$this->err_bme280_e1 = ($this->sys_error >> 9) & 0x1;
		$this->err_bme280_e2 = ($this->sys_error >> 10) & 0x1;

	}
	/*function getOV9655Error($type) {
		$error = ($this->sys_error >> 4) & 0x3;
		if($type == self::TYPE_INT)
			return $error;

		switch($error) {
			case 0: return $this->colorize($type, self::COL_GREEN, "OK");
			case 1: return $this->colorize($type, self::COL_RED, "I2C Error - Camera not found");
			case 2: return $this->colorize($type, self::COL_RED, "DMA abort - last buffer segment");
			case 3: return $this->colorize($type, self::COL_RED, "DMA FIFO error");
			case 4: return $this->colorize($type, self::COL_RED, "DMA stream transfer error");
			case 5: return $this->colorize($type, self::COL_RED, "DMA direct mode error");
		}
	}
	function getGPSStatus($type) {
		if($type == self::TYPE_INT)
			return $this->gps_lock;

		switch($this->gps_lock) {
			case 0: return $this->colorize($type, self::COL_GREEN, "GPS locked");
			case 1: return $this->colorize($type, self::COL_GREEN, "GPS locked - kept switched on");
			case 2: return $this->colorize($type, self::COL_RED, "GPS loss");
			case 3: return $this->colorize($type, self::COL_ORANGE, "Low Batt before switched on");
			case 4: return $this->colorize($type, self::COL_ORANGE, "Low Batt while switched on");
			case 5: return $this->colorize($type, self::COL_GREEN, "Data from memory");
			case 6: return $this->colorize($type, self::COL_RED, "GPS communication error");
		}
	}
	function getEVA7MError($type) {
		$error = ($this->sys_error >> 2) & 0x1;
		if($type == self::TYPE_INT)
			return $error;

		switch($error) {
			case 0: return $this->colorize($type, self::COL_GREEN, "OK");
			case 1: return $this->colorize($type, self::COL_RED, "Fail");
		}
	}
	function getI2cError($loc, $type) {
		$error = $loc == self::LOC_INT ? $this->sys_error & 0x1 : ($this->sys_error >> 1) & 0x1;
		if($type == self::TYPE_INT)
			return $error;

		switch($error) {
			case 0: return $this->colorize($type, self::COL_GREEN, "OK");
			case 1: return $this->colorize($type, self::COL_RED, "Fail");
		}
	}
	function getPAC1720Error($type) {
		$error = ($this->sys_error >> 3) & 0x1;
		if($type == self::TYPE_INT)
			return $error;

		switch($error) {
			case 0: return $this->colorize($type, self::COL_GREEN, "OK");
			case 1: return $this->colorize($type, self::COL_RED, "Fail");
		}
	}
	function getBME280Error($type, $loc) {
		switch($loc) {
			case self::LOC_INT1: $error = ($this->sys_error >> 7) & 0x1; break;
			case self::LOC_EXT1: $error = ($this->sys_error >> 8) & 0x1; break;
			case self::LOC_EXT2: $error = ($this->sys_error >> 9) & 0x1; break;
		}
		if($type == self::TYPE_INT)
			return $error;

		switch($error) {
			case 0: return $this->colorize($type, self::COL_GREEN, "OK");
			case 1: return $this->colorize($type, self::COL_RED, "Fail");
		}
	}

	private function colorize($type, $color, $str) {
		if($type == self::TYPE_HTML)
			return "<font color=\"$color\">$str</font>";
		elseif($type == self::TYPE_STR)
			return $str;
	}*/
}
?>

















