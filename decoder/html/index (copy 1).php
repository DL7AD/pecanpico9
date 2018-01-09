<?php
require "database.class.php";

$db = new MyDB();
?>
<!DOCTYPE html>
<html>
<head>
<style>
html {
	font: 10pt Monospace;
}
#map {
    position: absolute;
    top: 30px;
    left: 0;
    bottom: 50px;
    right: 0;
}
</style>
<script src="http://ajax.googleapis.com/ajax/libs/jquery/1.11.0/jquery.min.js" type="text/javascript"></script>
</head>
<body>
<div id="map"></div>
<script>

var map;
var items = [];

function drawItems() {
	$.getJSON("data.php", function(path) {
		// Remove old items
		while(true) {
			try {
				items.pop().setMap(null);
			} catch(err) {
				break;
			}
		}

		// Add new items on map
		var last = null;
		$.each(path, function(index, value) {
			// Point
			if(value.img) {
				var marker = new google.maps.Marker({
					position: new google.maps.LatLng(value.lat, value.lng),
					icon: {
						path: google.maps.SymbolPath.CIRCLE,
						scale: value.img ? 4 : 1.5,
						strokeColor: value.img ? '#000080' : value.org == 'log' ? '#FF0000' : '#008000'
					},
					map: map,
				});
				items.push(marker);

				// Image Info Window

				var infowindow = new google.maps.InfoWindow({
					content: '<img src="' + value.img + '" width="512" height="384" />'
				});
				marker.addListener('mouseover', function() {
					infowindow.open(map, marker);
				});
				marker.addListener('mouseout', function() {
					infowindow.close();
				});
			}

			// Line between points
			if(last) {
				if(last.lat != last.lng || value.lat != value.lng) {
					var line = new google.maps.Polyline({
						path: [last,value],
						geodesic: true,
						strokeColor: last.org == 'log' || value.org == 'log' ? '#FF0000' : '#008000',
						strokeOpacity: 0.4,
						strokeWeight: 5,
						map: map
					});
					console.log(last,value);
				}
				items.push(line);
			}
			last = value;
		});
	});
}
function initMap() {
	map = new google.maps.Map(document.getElementById('map'), {
		zoom: 12,
		center: new google.maps.LatLng(52.45,13.5),
		gestureHandling: 'greedy'
	});

	drawItems();
	window.setInterval(drawItems, 10000);
}
</script>
<script async defer src="https://maps.googleapis.com/maps/api/js?key=AIzaSyCrrxJc6mu5DjFZVHiFqhFMO7JJg2g89Y8&callback=initMap"></script>
</body>
</html>

