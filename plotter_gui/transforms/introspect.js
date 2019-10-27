function Dic()
{
  this.val = 0.0
  Number.call(this.val, 0.0);
  this.current = "";
  this.level = 0;
  this.keys = [];
}

Dic.prototype = Object.create(Number.prototype);
Dic.prototype.constructor = Dic;
Dic.prototype.valueOf = function() {
  return this.val;
};
Dic.prototype.toString = function() {
  return this.keys;
};

const spy = {
  get: function(target, prop, receiver) {
      console.log(prop)

      if (prop in target || typeof prop === 'symbol')
        return Reflect.get(target,prop,receiver)

      if (receiver.level == 0)
          target.current = " "
      target.current = target.current+"/"+prop
      target.keys.push(target.current)

      r = Object.create(receiver);
      r.level += 1;

      return r
    }
};

dic = new Dic()
dic1 = new Dic()
const proxy = new Proxy(dic, spy);
const proxy1 = new Proxy(dic1, spy);
console.log(dic + dic)
console.log(dic.level)
console.log(proxy +1 )

f = function(o) {return o.pose.x.z + o.pose.y;}
console.log(proxy1.a.b.valueOf());
//console.log(typeof proxy1.a.b);
k = f(proxy)
//h = f + g
console.log(dic.toString());
console.log(dic1.toString());
