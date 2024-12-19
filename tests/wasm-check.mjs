import puppeteer from 'puppeteer';

const browser = await puppeteer.launch({
  headless: "new",
  args: [
    '--allow-file-access-from-files',
    '--allow-file-access',
    '--no-sandbox',
  ]
})
try {
  const page = await browser.newPage();
  await page.setBypassCSP(true);
  page.on('console', message => console.log(`Console ${message.type()}: ${message.text()}`));
  page.on('pageerror', error => console.log('🔴 Page error:', error.message));
  page.on('requestfailed', request => console.log('🔴 Failed request:', request.url()));

  await page.goto(new URL('./wasm-check.html', import.meta.url), {
    waitUntil: 'networkidle0'
  });

  await page.waitForFunction(() => {
    const element = document.getElementById('output');
    return element && element.innerText.trim() !== '';
  }, {
    timeout: 3000,  // 3 second timeout
    polling: 100    // Check every 100ms
  });

  console.log(await page.$eval('#output', el => el.innerText));
} catch (error) {
  console.error('Error:', error);
} finally {
  await browser.close();
}
