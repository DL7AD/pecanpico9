<?php
error_reporting(E_ALL ^ E_NOTICE);

require_once "Database.class.php";
require_once "helper.inc.php";
?>
<!DOCTYPE html>
<html>
<head>
<title>Balloon Health</title>
<?php

$db = Database::getInstance();
if(array_key_exists('call', $_GET)) {
	$tracker = new Tracker($_GET['call']);
	$tel = $tracker->getLastTelemetry();
}

$range = isset($_GET['range']) && is_numeric($_GET['range']) ? $_GET['range'] : 86400;
?>
<link href="style.css" rel="stylesheet" type="text/css">
<script src="https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js"></script>

