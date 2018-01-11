<?php
class Telemetry {
	function __construct($sqlResult) {
		$this->reset = $sqlResult['reset'];
		$this->id = $sqlResult['id'];
		$this->org = $sqlResult['org'];

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
}
?>

