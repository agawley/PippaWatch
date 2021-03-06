Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('showConfiguration', function() {
  var url = 'https://gawley.net/vsw/configv2.1.html';
  console.log('Showing configuration page: ' + url);

  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log('Configuration page returned: ' + JSON.stringify(configData));

  var dict = {};
  dict['KEY_INVERTED'] = configData['inverted'] ? 1 : 0;  // Send a boolean as an integer
  dict['KEY_SHOW_STEP_COUNT'] = configData['show_step_count'] ? 1 : 0;  // Send a boolean as an integer
  dict['KEY_STEP_TARGET'] = parseInt(configData['step_target']);
  
  // Send to watchapp
  Pebble.sendAppMessage(dict, function() {
    console.log('Send successful: ' + JSON.stringify(dict));
  }, function() {
    console.log('Send failed!');
  });
});