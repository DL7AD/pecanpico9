<?php
require_once "Tracker.class.php";

class Database extends SQLite3 {
	private static $instance = null;

	function __construct() {
		$this->open("/src/pecanpico9/decoder/decoder.sqlite");

		if($this->lastErrorCode())
			echo $this->lastErrorMsg();
	}

	static function getInstance() {
		if(self::$instance == null)
			self::$instance = new Database();
		return self::$instance;
	}

	function close() {
		parent::close();
	}

	function getTracker() {
		$tracker = array();

		$query = $this->query("
			SELECT call,MAX(rxtime)
			FROM (
				SELECT call,rxtime FROM position
				UNION ALL
				SELECT call,rxtime FROM image
			)
			GROUP BY call
			ORDER BY rxtime DESC
		");
		while($row = $query->fetchArray(SQLITE3_ASSOC))
			$tracker[] = new Tracker($row['call']);

		return $tracker;
	}
}
?>








