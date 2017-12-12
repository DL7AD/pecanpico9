<?php
class MyDB extends SQLite3 {
	function __construct() {
		$this->open("/src/pecanpico9/decoder/decoder.sqlite");

		if($this->lastErrorCode())
			echo $this->lastErrorMsg();
	}
	function getCallsigns() {
		$calls = array();

		$query = $this->query("SELECT call FROM position GROUP BY call");
		while($row = $query->fetchArray(SQLITE3_ASSOC))
			$calls[] = $row['call'];

		$query = $this->query("SELECT call FROM image GROUP BY call");
		while($row = $query->fetchArray(SQLITE3_ASSOC))
			if(!in_array($row['call'], $calls))
			$calls[] = $row['call'];

		return $calls;
	}
	function getRoute($callsign) {
		$route = array();

		$stmt = $this->prepare("
			SELECT position.time,ROUND(position.lat,5) as lat,ROUND(position.lon,5) as lng,position.alt,org,
			'images/' || REPLACE(image.call,'-','') || '-' || image.time || '-' || image.imageID || '.jpg' AS img
			FROM position
			LEFT JOIN image
			ON position.time = image.time
			WHERE position.call = :call
			AND position.lat != 0
			AND position.lon != 0
			AND position.isnew = 1
			-- AND position.time + 86400*14 > CAST(strftime('%s', 'now') as DECIMAL)
			GROUP BY position.call,position.time
			ORDER BY position.time ASC
		");
		$stmt->bindValue(':call', $callsign, SQLITE3_TEXT);
		$query = $stmt->execute();

		while($row = $query->fetchArray(SQLITE3_ASSOC))
			$route[] = $row;

		return $route;
	}
}
?>
