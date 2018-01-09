<?php
function time_format($time) {
	if(!$time)
		return "never";

	if($time < 3600)
		return floor($time/60) . "m" . ($time%60) . "s ago";
	else
		return floor($time/3600) . "h" . floor(($time/60)%60) . "m ago";
}
?>
