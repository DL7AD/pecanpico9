<?php
require "database.class.php";

$data = array();
$db = new MyDB();
foreach($db->getCallsigns() as $callsign)
	foreach($db->getRoute($callsign) as $point)
		$data[] = $point;

header("Content-Type: application/json");
echo json_encode($data);
?>

