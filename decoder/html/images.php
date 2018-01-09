<?php
include "header.inc.php";
?>
<script type="text/javascript">

lastrxtime = <?=time()-$range?>;
call = '<?=$_GET['call']?>';
var last = null;

function number_format(number, decimals, decPoint, thousandsSep) { // eslint-disable-line camelcase
	//  discuss at: http://locutus.io/php/number_format/
	// original by: Jonas Raoni Soares Silva (http://www.jsfromhell.com)
	// improved by: Kevin van Zonneveld (http://kvz.io)
	// improved by: davook
	// improved by: Brett Zamir (http://brett-zamir.me)
	// improved by: Brett Zamir (http://brett-zamir.me)
	// improved by: Theriault (https://github.com/Theriault)
	// improved by: Kevin van Zonneveld (http://kvz.io)
	// bugfixed by: Michael White (http://getsprink.com)
	// bugfixed by: Benjamin Lupton
	// bugfixed by: Allan Jensen (http://www.winternet.no)
	// bugfixed by: Howard Yeend
	// bugfixed by: Diogo Resende
	// bugfixed by: Rival
	// bugfixed by: Brett Zamir (http://brett-zamir.me)
	//  revised by: Jonas Raoni Soares Silva (http://www.jsfromhell.com)
	//  revised by: Luke Smith (http://lucassmith.name)
	//    input by: Kheang Hok Chin (http://www.distantia.ca/)
	//    input by: Jay Klehr
	//    input by: Amir Habibi (http://www.residence-mixte.com/)
	//    input by: Amirouche
	number = (number + '').replace(/[^0-9+\-Ee.]/g, '');
	var n = !isFinite(+number) ? 0 : +number;
	var prec = !isFinite(+decimals) ? 0 : Math.abs(decimals);
	var sep = (typeof thousandsSep === 'undefined') ? ',' : thousandsSep;
	var dec = (typeof decPoint === 'undefined') ? '.' : decPoint;
	var s = '';
	var toFixedFix = function (n, prec) {
		var k = Math.pow(10, prec);
		return '' + (Math.round(n * k) / k).toFixed(prec);
	}
	// @todo: for IE parseFloat(0.55).toFixed(0) = 0;
	s = (prec ? toFixedFix(n, prec) : '' + Math.round(n)).split('.');
	if(s[0].length > 3) {
		s[0] = s[0].replace(/\B(?=(?:\d{3})+(?!\d))/g, sep);
	}
	if((s[1] || '').length < prec) {
		s[1] = s[1] || '';
		s[1] += new Array(prec - s[1].length + 1).join('0');
	}
	return s.join(dec);
}

function time_format(time) {
	if(time == undefined)
		return "never";

	if(time < 3600)
		return Math.floor(time/60) + "m" + (time%60) + "s ago";
	else
		return Math.floor(time/3600) + "h" + Math.floor((time/60)%60) + "m ago";
}

function loadRecentData() {
	$.getJSON("ajax/telemetry.php?call=" + call + "&from=" + lastrxtime, function(json) {
		images = json['images'];

		if(images.length) {
			lastrxtime = images[images.length-1].time_last+1;

			$.each(images, function(key, data) {
				// Remove old div
				$("#img_" + data['id']).remove();

				// Process images
				$('#images').prepend("<div class=\"pic\" id=\"img_" + data['id'] + "\">"
				 + "<img src=\"images/" + data['call'].replace('-','') + "-" + data['id'] + ".jpg?packetID=" + data['packetID'] + "\"><br>"
				 + "Last packet " + time_format(json['time']-data['time_last']) + ", " + number_format(data['count']) + " packets, "
				 + number_format(data['packetID']-data['count']+1) + " lost" + "<br>ImageID " + number_format(data['imageID']) + ", ServerID "
				 + number_format(data['id']) + "</div>");
			});
		}
	});
}
function loadImages() {
	loadRecentData();
	setInterval(loadRecentData, 1000);
}
</script>
</head>
<body onload="loadImages()">

<?php
include "sidebar.inc.php";
?>

<table>
	<tr>
		<td>
			&nbsp;Range:
			<a href="?call=<?=$_GET['call']?>&range=3600">1h</a>
			<a href="?call=<?=$_GET['call']?>&range=10800">3h</a>
			<a href="?call=<?=$_GET['call']?>&range=21600">6h</a>
			<a href="?call=<?=$_GET['call']?>&range=43200">12h</a>
			<a href="?call=<?=$_GET['call']?>&range=86400">24h</a>
			<a href="?call=<?=$_GET['call']?>&range=172800">2d</a>
			<a href="?call=<?=$_GET['call']?>&range=259200">3d</a>
			<a href="?call=<?=$_GET['call']?>&range=432000">5d</a>
			<a href="?call=<?=$_GET['call']?>&range=604800">7d</a>
			<a href="?call=<?=$_GET['call']?>&range=1209600">14d</a>
			<a href="?call=<?=$_GET['call']?>&range=1814400">21d</a>
			<a href="?call=<?=$_GET['call']?>&range=2592000">30d</a>
		</td>
	</tr>
</table>

<div style="width:1330px;float:left;" id="images"></div>

</body>
</html>
<?php
include "footer.inc.php";
?>

