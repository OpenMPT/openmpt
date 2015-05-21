
var Module = {
 'preInit': function(text) {
  FS.mkdir('/test');
  FS.mount(NODEFS, {'root': '../test/'}, '/test');
 }
};
