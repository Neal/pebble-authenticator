Pebble.addEventListener('ready', function(e) {});

Pebble.addEventListener('showConfiguration', function(e) {
	var options = JSON.parse(window.localStorage.getItem('options'));
	console.log('read options: ' + JSON.stringify(options));
	console.log('showing configuration');
	var uri = 'https://rawgithub.com/Neal/pebble-authenticator/master/html/configuration.html?' +
				'timezone=' + encodeURIComponent(options['timezone']) +
				'&vib_warn=' + encodeURIComponent(options['vib_warn']) +
				'&vib_renew=' + encodeURIComponent(options['vib_renew']);
	Pebble.openURL(uri);
});

Pebble.addEventListener('webviewclosed', function(e) {
	console.log('configuration closed');
	if (e.response) {
		var options = JSON.parse(decodeURIComponent(e.response));
		console.log('storing options: ' + JSON.stringify(options));
		window.localStorage.setItem('options', JSON.stringify(options));
		Pebble.sendAppMessage(options,
			function(e) {
				console.log('successfully sent options to pebble');
			},
			function(e) {
				console.log('failed to send options to pebble. Error: ' + e.error.message);
			}
		);
	} else {
		console.log('no options received');
	}
});
