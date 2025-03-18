document.addEventListener('DOMContentLoaded', () => {
  renderFooterTemplate();
});

document.querySelector('#btnRefresh').addEventListener('click', async () => {
  const networksLoader = document.querySelector('#networksLoader');
  const networksFounded = document.querySelector('#networksFounded');
  const list = document.querySelector('#list');

  networksFounded.classList.add('hidden');
  networksLoader.classList.remove('hidden');

  list.textContent = '';

  networks = await getNetworks();
  renderList(networks);

  networksLoader.classList.add('hidden');
  networksFounded.classList.remove('hidden');
});

document.querySelector('#btnScan').addEventListener('click', async () => {
  const networksLoader = document.querySelector('#networksLoader');

  document.querySelector('#networksHero').classList.add('hidden');
  networksLoader.classList.remove('hidden');

  networks = await getNetworks();
  renderList(networks);

  networksLoader.classList.add('hidden');
  document.querySelector('#networksFounded').classList.remove('hidden');
});

document.querySelector('#form').addEventListener('submit', async (e) => {
  e.preventDefault();

  const ssid = document.querySelector('#ssidInput').value;
  const password = document.querySelector('#passwordInput').value;

  try {
    await fetch('/setup/wifi', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ ssid, password }),
    });

    console.log('Datos guardados');
  } catch (error) {
    console.log(error);
  }
});

function renderFooterTemplate() {
  const footer = document.querySelector('#footer');
  const footerTemplate = document.querySelector('#footerTemplate');

  const templateClone = footerTemplate.content.firstElementChild.cloneNode(true);

  const year = new Date().getFullYear();
  templateClone.querySelector('#footerYear').textContent = year;

  footer.appendChild(templateClone);
}

function getListItemNode(network) {
  const list = document.querySelector('#list');
  const listItemTemplate = document.querySelector('#listItemTemplate');

  const templateClone = listItemTemplate.content.firstElementChild.cloneNode(true);

  // SSID
  templateClone.querySelector('#listItemSSID').textContent = network.ssid;

  const button = templateClone.querySelector('#btnListItem');
  button.setAttribute('data-ssid', network.ssid);

  button.addEventListener('click', (e) => {
    const ssidInput = document.querySelector('#ssidInput');
    const passwordInput = document.querySelector('#passwordInput');
    const ssid = e.target.closest('button').getAttribute('data-ssid');

    ssidInput.value = ssid;

    passwordInput.focus();
    passwordInput.scrollIntoView({ behavior: 'smooth', block: 'center' });
  });

  // RRSI
  templateClone
    .querySelector('#listItemRRSI')
    .setAttribute('data-level', parseRRSILevel(network.rssi));

  // Secure
  if (network.secure) {
    templateClone.querySelector('#listItemNotSecure').classList.add('hidden');
  } else {
    templateClone.querySelector('#listItemSecure').classList.add('hidden');
  }

  return templateClone;
}

function renderList(networks) {
  const list = document.querySelector('#list');
  const fragment = document.createDocumentFragment();

  list.textContent = '';

  networks.forEach((network) => {
    fragment.appendChild(getListItemNode(network));
  });

  list.appendChild(fragment);
}

async function getNetworks() {
  const response = await fetch('/scan');
  const networks = await response.json();
  return networks;
}

function parseRRSILevel(rrsi) {
  if (rrsi >= -50) return 4;
  else if (rrsi >= -60) return 3;
  else if (rrsi >= -70) return 2;
  else if (rrsi >= -80) return 1;
  else return 0;
}
