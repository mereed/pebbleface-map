Pebble.addEventListener('ready', function(e){
  console.log('JS ready');
  
  setTimeout(function(){
    var x = new Date();
    console.log("Time zone offset: "+x.getTimezoneOffset());
    
    var dict = {
      'TIME_OFFSET_KEY':x.getTimezoneOffset()
    };

    Pebble.sendAppMessage(dict,
      function(e) {
        console.log('Send successful.');
      },
      function(e) {
        console.log('Send failed!'+ JSON.stringify(e));
      }
    );
  }, 1);
});