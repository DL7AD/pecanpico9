<div class="inner" style="width:270px;height:1535px;float:left;">
	<?php
	$lasthour = false;
	$today = false;
	$yesterday = false;
	$older = false;

	$trackers = $db->getTracker();
	foreach($trackers as $tr) {

		$cnt = $tr->getPacketCount();

		echo "<div class=\"call\">
		<b><a href=\"telemetry.php?call=" . $tr->getCall() . "\">" . $tr->getCall() . "</a> ...
		<a href=\"map.php?call=" . $tr->getCall() . "\">Map</a>
		<a href=\"images.php?call=" . $tr->getCall() . "\">Images</a></b><br>
		Last Activity: " . time_format(max($tr->getLastActivity())) . "<br>
		Packets: " . number_format($cnt['img']['cnt300'] + $cnt['pos']['cnt300']) . " (5m), " . number_format($cnt['img']['cnt3600'] + $cnt['pos']['cnt3600']) . " (1h)
		</div>";

	}
	?>
</div>
