const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const path = require('node:path');
const html = fs.readFileSync(path.join(__dirname, '../../../android/app/src/main/assets/launcher/index.html'), 'utf8');
const options = html.slice(html.indexOf('const TABS='), html.indexOf('const PAD_BASE='));
const config = html.slice(html.indexOf('function buildConfig()'), html.indexOf('const R={screen:'));
assert(options && config);
const context = vm.createContext({assert});
vm.runInContext(`const OFFON=['Off','On']; ${options}
let P={settings:{...DEF}};
${config}
assert.deepEqual(Array.from(OPTS.padResponse.opts), ['Classic','Linear','Dynamic']);
assert.equal(P.settings.padResponse, 0);
assert.equal(P.settings.padDeadzone, 20);
assert.equal(P.settings.padAds, 100);
assert(buildConfig().includes('seta bo1_pad_response 0'));
assert(buildConfig().includes('seta bo1_pad_right_deadzone 0.20'));
assert(buildConfig().includes('seta bo1_pad_ads_scale 1.00'));
for (let mode=0;mode<=2;mode++) {
  for (const deadzone of [0,7,50]) {
    for (const ads of [10,75,100,200]) {
      P.settings={...DEF,padResponse:mode,padDeadzone:deadzone,padAds:ads};
      const cfg=buildConfig();
      assert(cfg.includes('seta bo1_pad_response '+mode));
      assert(cfg.includes('seta bo1_pad_right_deadzone '+(deadzone/100).toFixed(2)));
      assert(cfg.includes('seta bo1_pad_ads_scale '+(ads/100).toFixed(2)));
      assert(!cfg.includes('NaN') && !cfg.includes('undefined'));
      assert(!cfg.includes('seta aim_') && !cfg.includes('set aim_'));
    }
  }
}
`, context);
console.log('PASS: launcher Classic defaults and all response modes, deadzone/ADS ranges, config conversion, unchanged aim-assist dvars');
