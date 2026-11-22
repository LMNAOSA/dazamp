from pathlib import Path
from urllib.parse import urlparse
import json
import struct
from playwright.sync_api import sync_playwright
BASE = Path(__file__).resolve().parents[1]
errors=[]
with sync_playwright() as p:
    browser = p.chromium.launch(headless=True, executable_path='/usr/bin/chromium', args=['--no-sandbox', '--autoplay-policy=no-user-gesture-required'])
    page = browser.new_page(viewport={'width':1440,'height':1000}, device_scale_factor=1)
    page.on('pageerror', lambda error: errors.append(str(error)))
    page.evaluate("Object.defineProperty(window, 'localStorage', {value:{_s:{},getItem(k){return this._s[k]||null},setItem(k,v){this._s[k]=String(v)}},configurable:true})")
    page.set_content((BASE/'DAZAMP-demo.html').read_text(), wait_until='load')
    assert page.locator('.knob').count()==4
    assert page.locator('#presetSelect option').count()==6
    assert page.locator('#tunerPanel').is_hidden()
    page.locator('#powerButton').click()
    page.wait_for_timeout(900)
    assert page.locator('#engineState').inner_text()=='ENGINE LIVE'
    output = page.locator('#peakLabel').inner_text()
    assert '∞' not in output, f'Synthetic riff silent: {output}'
    page.locator('#dryCheckButton').click()
    page.wait_for_timeout(240)
    dry_db = float(page.locator('#peakLabel').inner_text().split(' ')[0])
    assert page.locator('#dryCheckButton').get_attribute('aria-pressed')=='true'
    assert 'bypassed' in page.locator('#sourceStatus').inner_text().lower()
    page.locator('#dryCheckButton').click()
    page.wait_for_timeout(240)
    wet_db = float(page.locator('#peakLabel').inner_text().split(' ')[0])
    assert page.locator('#dryCheckButton').get_attribute('aria-pressed')=='false'
    assert abs(wet_db-dry_db)>1, f'Bypass not changing output level: {dry_db}, {wet_db}'
    page.locator('#monoButton').click()
    assert page.locator('#monoButton').get_attribute('aria-pressed')=='true'
    assert page.locator('#sourceStatus').inner_text().startswith('MONO CHECK')
    page.locator('#monoButton').click()
    assert page.locator('#monoButton').get_attribute('aria-pressed')=='false'
    page.locator('#tunerButton').click()
    assert page.locator('#tunerPanel').is_visible()
    page.locator('#toggleAdvanced').click()
    assert page.locator('#advancedPanel').is_visible()
    page.locator('#highCut').fill('4500')
    assert page.locator('#highCutOut').inner_text()=='4500 Hz'
    # Load a genuine WAV-format impulse to validate the custom cabinet path.
    impulse = [int(20000 * (0.92**i)) for i in range(128)]
    pcm = struct.pack('<'+'h'*len(impulse), *impulse)
    riff_size = 36+len(pcm)
    wav = (b'RIFF'+struct.pack('<I',riff_size)+b'WAVEfmt '+struct.pack('<IHHIIHH',16,1,1,48000,96000,2,16)+b'data'+struct.pack('<I',len(pcm))+pcm)
    page.locator('#irInput').set_input_files({'name':'test-cab.wav','mimeType':'audio/wav','buffer':wav})
    page.wait_for_timeout(250)
    assert 'Custom IR active' in page.locator('#irStatus').inner_text(), page.locator('#irStatus').inner_text()
    assert page.locator('#cabMix').is_disabled()
    page.locator('#bButton').click()
    assert 'selected' in (page.locator('#bButton').get_attribute('class') or '')
    page.locator('#aButton').click()
    assert 'selected' in (page.locator('#aButton').get_attribute('class') or '')
    sample={'schema':'dazamp-preset-bank-v1','presets':[{'name':'MY TEST PRESET','settings':{'tight':72,'grind':81,'body':69,'space':20}}]}
    page.locator('#importBankInput').set_input_files({'name':'preset.json','mimeType':'application/json','buffer':json.dumps(sample).encode()})
    page.wait_for_function("document.querySelector('#sourceStatus').textContent.includes('Imported 1 new presets')")
    assert page.locator('#presetSelect option').count()==7
    with page.expect_download() as d:
        page.locator('#exportBank').click()
    download=d.value
    assert download.suggested_filename=='DAZAMP-preset-bank.json'
    download.save_as(str(BASE/'tests/export-smoke.json'))
    exported=json.loads((BASE/'tests/export-smoke.json').read_text())
    assert exported['presets'][0]['name']=='MY TEST PRESET'
    page.locator('#toggleAdvanced').click()
    page.locator('#tunerButton').click()
    page.screenshot(path=str(BASE/'preview-desktop.png'), full_page=True)
    assert not errors, f'Page exceptions: {errors}'
    print('PASS: live Web Audio, output level',output, '| stereo/mono routing, tuner display, advanced controls, snapshots, bank import/export, page errors=0')
    mobile=browser.new_page(viewport={'width':390,'height':844},device_scale_factor=1)
    mobile.set_content((BASE/'DAZAMP-demo.html').read_text(), wait_until='load')
    mobile.screenshot(path=str(BASE/'preview-mobile.png'),full_page=True)
    assert mobile.evaluate('document.documentElement.scrollWidth <= window.innerWidth'), 'Mobile overflow'
    assert mobile.locator('.knob').count()==4
    print('PASS: mobile 390px no horizontal overflow')
    browser.close()
