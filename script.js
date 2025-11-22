fetch('packages.json')
  .then(response => response.json())
  .then(data => {
    const container = document.getElementById('paquetes');
    data.forEach(pkg => {
      const div = document.createElement('div');
      div.innerHTML = `
        <h3>${pkg.nombre} (${pkg.version})</h3>
        <p>${pkg.descripcion}</p>
        <a href="${pkg.url}" target="_blank">Descargar</a>
      `;
      container.appendChild(div);
    });
  })
  .catch(err => console.error("Error cargando paquetes:", err));
