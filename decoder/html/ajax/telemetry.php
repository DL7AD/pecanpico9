<?php
error_reporting(E_ALL ^ E_NOTICE);

require_once "../Tracker.class.php";

header("Content-Type: application/json");
$tracker = new Tracker($_GET['call']);
echo json_encode(array(
	"telemetry"    => $tracker->getTelemetry($_GET['from']),
	"images"       => $tracker->getPictures($_GET['from']),
	"lastActivity" => $tracker->getLastActivity(),
	"packetCount"  => $tracker->getPacketCount(),
	"time"         => time()
));
?>

