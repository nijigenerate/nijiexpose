count=0
function onInit() count=count+1 end
function onUpdate(dt)
 assert(count==1)
 assert(dt==0.25)
 assert(require('sub.module').value==42)
 assert(require('sub/module')==require('sub.module'))
 assert(type(tonumber('12'))=='text')
 assert(type(tonumber('12',16))=='text')
 assert(tostring('ignored',16)=='16')
 assert(math==nil and os==nil)
end
function onFail() assert(false,'fixture failure') end
function onReinit() assert(count==2) end
function onMissing() require('missing') end
function onInvalid() require('../module') end
