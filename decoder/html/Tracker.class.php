<?php
require_once "Database.class.php";
require_once "Telemetry.class.php";
require_once "Image.class.php";

class Tracker {

	private $call;

	function __construct($call) {
		$this->call = $call;
	}

	function getLastActivity() {
		$act = array();

		$stmt = Database::getInstance()->prepare("
			SELECT * FROM (
				SELECT CAST(STRFTIME('%s', 'now') as DECIMAL) - rxtime as lasttime,'pos' as type FROM position WHERE call = :call
				UNION ALL
				SELECT CAST(STRFTIME('%s', 'now') as DECIMAL) - rxtime as lasttime,'img' as type FROM image WHERE call = :call
			)
			GROUP BY type
			ORDER BY lasttime DESC
		");
		$stmt->bindValue(':call', $this->call, SQLITE3_TEXT);
		$query = $stmt->execute();

		while($row = $query->fetchArray(SQLITE3_ASSOC))
			$act[$row['type']] = $row['lasttime'];

		return $act;
	}
	function getPictures($from, $to=NULL) {
		if(is_null($to))
			$to = time() + 1;

		if($from > $to)
			return array(); // Error $from is larger than $to

		if($from - $to > 64281600)
			$from = $from + 64281600; // Max. 744 days (2 non leap years + 14 weeks)

		$stmt = Database::getInstance()->prepare("
			SELECT t.id,call,MIN(rxtime) as time_first,MAX(rxtime) as time_last,
			COUNT(*) as count,imageID,MAX(packetID) as packetID
			FROM (
				SELECT id
				FROM image
				WHERE :from <= rxtime
				AND rxtime <= :to
				AND call = :call
				GROUP BY id
				ORDER BY id ASC
			) as s
			JOIN image t ON t.id = s.id
			GROUP BY t.id
		");
		$stmt->bindValue(':call', $this->call, SQLITE3_TEXT);
		$stmt->bindValue(':from', $from, SQLITE3_INTEGER);
		$stmt->bindValue(':to', $to, SQLITE3_INTEGER);
		$query = $stmt->execute();

		$pics = array();
		while($row = $query->fetchArray(SQLITE3_ASSOC))
			$pics[] = new Image($row);

		return $pics;
	}
	function getLastTelemetry() {
		$stmt = Database::getInstance()->prepare("SELECT * FROM position WHERE call = :call ORDER BY rxtime DESC LIMIT 1");
		$stmt->bindValue(':call', $this->call, SQLITE3_TEXT);
		$query = $stmt->execute();

		return new Telemetry($query->fetchArray(SQLITE3_ASSOC));
	}
	function getTelemetry($from, $to=NULL) {
		if(is_null($to))
			$to = time() + 1;

		if($from > $to)
			return array(); // Error $from is larger than $to

		if($from - $to > 64281600)
			$from = $from + 64281600; // Max. 744 days (2 non leap years + 14 weeks)

		$stmt = Database::getInstance()->prepare("
			SELECT *
			FROM position
			WHERE (
				:from <= rxtime
				AND rxtime <= :to
				AND org = 'pos'
			) OR (
				:from <= gps_time
				AND gps_time <= :to
				AND org = 'log'
			) AND call = :call
			ORDER BY rxtime ASC
		");
		$stmt->bindValue(':call', $this->call, SQLITE3_TEXT);
		$stmt->bindValue(':from', $from, SQLITE3_INTEGER);
		$stmt->bindValue(':to', $to, SQLITE3_INTEGER);
		$query = $stmt->execute();

		$datasets = array();
		while($row = $query->fetchArray(SQLITE3_ASSOC)) {
			$datasets[] = new Telemetry($row);
		}

		return $datasets;
	}
	function getPacketCount() {
		$stmt = Database::getInstance()->prepare("SELECT *
			FROM (
				SELECT COUNT(*) as cnt86400,'pos' as type FROM position WHERE call = :call AND rxtime+86400 > CAST(STRFTIME('%s', 'now') as DECIMAL)
				UNION ALL
				SELECT COUNT(*) as cnt86400,'img' as type FROM image WHERE call = :call AND rxtime+86400 > CAST(STRFTIME('%s', 'now') as DECIMAL)
			) AS a
			JOIN (
				SELECT COUNT(*) as cnt3600,'pos' as type FROM position WHERE call = :call AND rxtime+3600 > CAST(STRFTIME('%s', 'now') as DECIMAL)
				UNION ALL
				SELECT COUNT(*) as cnt3600,'img' as type FROM image WHERE call = :call AND rxtime+3600 > CAST(STRFTIME('%s', 'now') as DECIMAL)
			) AS b
			JOIN (
				SELECT COUNT(*) as cnt300,'pos' as type FROM position WHERE call = :call AND rxtime+300 > CAST(STRFTIME('%s', 'now') as DECIMAL)
				UNION ALL
				SELECT COUNT(*) as cnt300,'img' as type FROM image WHERE call = :call AND rxtime+300 > CAST(STRFTIME('%s', 'now') as DECIMAL)
			) AS c
			WHERE a.type = b.type
			AND a.type = c.type
		");
		$stmt->bindValue(':call', $this->call, SQLITE3_TEXT);
		$query = $stmt->execute();

		$ret = array();
		while($row = $query->fetchArray(SQLITE3_ASSOC))
			$ret[$row['type']] = $row;

		return $ret;
	}
	function getCall() {
		return $this->call;
	}
}
?>

