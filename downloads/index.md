---
layout: page
title: Downloads
permalink: /download/
---

<style>
.downloads-badge {
    background: var(--wl);
    padding: 2px 8px;
    border-radius: 12px;
    font-size: 11px;
    margin-left: 8px;
    border: 1px solid var(--bd);
}

.downloads-asset-info {
    padding: 10px 12px;
    border-bottom: 1px solid var(--bd);
}

.downloads-asset-info a {
    text-decoration: none;
    color: var(--tx);
    display: block;
}

.downloads-asset-info a:hover {
    color: var(--ac);
}

.table-downloads {
    width: 100%;
    border-collapse: collapse;
    margin-top: 20px;
}

.table-downloads th, .table-downloads td {
    padding: 10px;
    text-align: left;
    border-bottom: 1px solid var(--bd);
}

.downloads-release-info {
    cursor: pointer;
}

.total-downloads {
    background: var(--wl);
    padding: 16px 24px;
    margin: 20px 0;
    text-align: center;
    border: 1px solid var(--bd);
    border-radius: 12px;
}

.total-downloads-value {
    font-size: 28px;
    font-weight: bold;
    color: var(--ac);
}

.loading {
    text-align: center;
    padding: 40px;
    color: var(--ac);
}

.badge-success {
    background: #28a74520;
    color: #28a745;
}

.badge-warning {
    background: #ffc10720;
    color: #ffc107;
}
</style>

<div id="total-downloads" class="total-downloads" style="display:none">
    Total Downloads: <span class="total-downloads-value" id="total-value"></span>
</div>

<div id="loading" class="loading" style="display:none">
    <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 24 24">
        <circle cx="12" cy="12" r="3" fill="currentColor"/>
        <g>
            <circle cx="4" cy="12" r="3" fill="currentColor"/>
            <circle cx="20" cy="12" r="3" fill="currentColor"/>
            <animateTransform attributeName="transform" calcMode="spline" dur="1s" keySplines=".36,.6,.31,1;.36,.6,.31,1" repeatCount="indefinite" type="rotate" values="0 12 12;180 12 12;360 12 12"/>
        </g>
    </svg>
</div>

<table id="releases-table" class="table-downloads" style="display:none">
    <thead>
        <tr>
            <th>Release</th>
            <th>Date</th>
            <th>Downloads</th>
        </tr>
    </thead>
    <tbody id="releases-body">
    </tbody>
</table>

<script>
function escapeHtml(str) {
    if (!str) return '';
    return str.replace(/[&<>]/g, function(m) {
        if (m === '&') return '&amp;';
        if (m === '<') return '&lt;';
        if (m === '>') return '&gt;';
        return m;
    });
}

function formatBytes(bytes) {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

async function getReleases() {
    let repo = 'zynomon/vex';
    
    document.getElementById('releases-body').innerHTML = '';
    document.getElementById('total-downloads').style.display = 'none';
    document.getElementById('releases-table').style.display = 'none';
    document.getElementById('loading').style.display = 'block';
    
    let totalDownloadCount = 0;
    let perPage = 100;
    let page = 1;
    let latest = true;
    
    while (true) {
        let response = await fetch(`https://api.github.com/repos/${repo}/releases?per_page=${perPage}&page=${page}`);
        if (!response.ok) break;
        
        let releases = await response.json();
        if (!Array.isArray(releases) || releases.length === 0) break;
        
        for (let i = 0; i < releases.length; i++) {
            let release = releases[i];
            if (!release.assets || release.assets.length === 0) continue;
            
            let downloadCount = 0;
            for (let asset of release.assets) {
                downloadCount += asset.download_count;
            }
            totalDownloadCount += downloadCount;
            
            let publishedAt = new Date(release.published_at);
            let releaseDate = publishedAt.toLocaleDateString('en-CA');
            
            let badge = '';
            if (latest) badge += '<span class="badge badge-success">Latest</span>';
            if (release.prerelease) badge += '<span class="badge badge-warning">Pre-release</span>';
            
            let assetsHtml = '';
            for (let asset of release.assets) {
                assetsHtml += `
                    <div class="downloads-asset-info">
                        <a href="${asset.browser_download_url}">
                            ${escapeHtml(asset.name)}
                            <span class="downloads-badge">${formatBytes(asset.size)}</span>
                            <span class="downloads-badge">
                                <img src="https://api.iconify.design/fluent-color:arrow-square-down-24.svg" width="14" height="14" style="vertical-align: middle; margin-right: 4px;">
                                ${asset.download_count.toLocaleString()}
                            </span>
                        </a>
                    </div>
                `;
            }
            
            let rowId = 'release-' + Date.now() + '-' + i;
            
            let row = `
                <tr class="downloads-release-info" onclick="toggleRow('${rowId}')">
                    <td>${escapeHtml(release.name)} ${badge}</td>
                    <td>${releaseDate}</td>
                    <td>${downloadCount.toLocaleString()}</td>
                </tr>
                <tr id="${rowId}" style="display:none">
                    <td colspan="3">
                        <div class="downloads-assets-container">
                            ${assetsHtml}
                        </div>
                    </td>
                </tr>
            `;
            
            document.getElementById('releases-body').insertAdjacentHTML('beforeend', row);
            latest = false;
        }
        
        document.getElementById('total-value').textContent = totalDownloadCount.toLocaleString();
        document.getElementById('total-downloads').style.display = 'block';
        document.getElementById('releases-table').style.display = 'table';
        document.getElementById('loading').style.display = 'none';
        
        if (releases.length < perPage) break;
        page++;
    }
}

function toggleRow(id) {
    let row = document.getElementById(id);
    if (row.style.display === 'none') {
        row.style.display = 'table-row';
    } else {
        row.style.display = 'none';
    }
}

getReleases();
</script>