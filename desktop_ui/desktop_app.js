/**
 * ============================================================
 *  desktop_app.js — Windows Desktop Bridge for C++ Engine
 *  Pharma Inventory Management System
 *
 *  Architecture:
 *    Desktop UI (HTML/CSS/JS) <──> Node.js Bridge <──> main.exe (Compiled C++ OOP Engine)
 * ============================================================
 */

const http = require('http');
const fs = require('fs');
const path = require('path');
const { execFile, spawn } = require('child_process');

const PORT = 4000;
const ROOT_DIR = path.join(__dirname, '..');
const CPP_DIR = path.join(ROOT_DIR, 'Cpp Files');
const EXE_PATH = path.join(CPP_DIR, 'main.exe');
const UI_DIR = __dirname;

// Helper: Run C++ executable and return stdout
function runCppEngine(args) {
    return new Promise((resolve, reject) => {
        execFile(EXE_PATH, args, { cwd: CPP_DIR }, (error, stdout, stderr) => {
            if (error) {
                resolve({
                    success: false,
                    error: stderr || error.message,
                    command: `.\\main.exe ${args.join(' ')}`
                });
            } else {
                let parsed = stdout.trim();
                try {
                    parsed = JSON.parse(stdout.trim());
                } catch (e) {
                    // Raw string if not JSON
                }
                resolve({
                    success: true,
                    data: parsed,
                    command: `.\\main.exe ${args.join(' ')}`
                });
            }
        });
    });
}

// MIME types for static files
const MIME_TYPES = {
    '.html': 'text/html',
    '.css': 'text/css',
    '.js': 'application/javascript',
    '.png': 'image/png',
    '.svg': 'image/svg+xml'
};

const server = http.createServer(async (req, res) => {
    const url = new URL(req.url, `http://localhost:${PORT}`);

    // CORS & JSON Headers
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

    if (req.method === 'OPTIONS') {
        res.writeHead(204);
        res.end();
        return;
    }

    // ────────────────────────────────────────────────────────
    // API Endpoints — Invoking compiled C++ binary
    // ────────────────────────────────────────────────────────
    if (url.pathname === '/api/stock') {
        const result = await runCppEngine(['--api-stock']);
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify(result));
        return;
    }

    if (url.pathname === '/api/catalog') {
        const result = await runCppEngine(['--api-catalog']);
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify(result));
        return;
    }

    if (url.pathname === '/api/suppliers') {
        const result = await runCppEngine(['--api-suppliers']);
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify(result));
        return;
    }

    if (url.pathname === '/api/alerts') {
        const result = await runCppEngine(['--api-alerts']);
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify(result));
        return;
    }

    if (url.pathname === '/api/reports') {
        const result = await runCppEngine(['--api-reports']);
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify(result));
        return;
    }

    if (url.pathname === '/api/sell' && req.method === 'POST') {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', async () => {
            const data = JSON.parse(body || '{}');
            const result = await runCppEngine([
                '--api-sell',
                data.stockID || '',
                String(data.quantity || 1),
                data.customerName || 'Customer'
            ]);
            res.writeHead(200, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify(result));
        });
        return;
    }

    if (url.pathname === '/api/receive' && req.method === 'POST') {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', async () => {
            const data = JSON.parse(body || '{}');
            const result = await runCppEngine([
                '--api-receive',
                data.medicineID,
                data.supplierID,
                data.batchNumber,
                String(data.quantity),
                String(data.cost),
                data.mfgDate,
                data.expiryDate,
                String(data.minStock)
            ]);
            res.writeHead(200, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify(result));
        });
        return;
    }

    if (url.pathname === '/api/add-medicine' && req.method === 'POST') {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', async () => {
            const data = JSON.parse(body || '{}');
            const result = await runCppEngine([
                '--api-add-med',
                data.type,
                data.id,
                data.name,
                data.manufacturer,
                String(data.price),
                String(data.field1),
                String(data.field2)
            ]);
            res.writeHead(200, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify(result));
        });
        return;
    }

    if (url.pathname === '/api/add-supplier' && req.method === 'POST') {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', async () => {
            const data = JSON.parse(body || '{}');
            const result = await runCppEngine([
                '--api-add-sup',
                data.id,
                data.company,
                data.name,
                data.phone,
                data.email || 'N/A',
                data.address || 'N/A'
            ]);
            res.writeHead(200, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify(result));
        });
        return;
    }

    // ────────────────────────────────────────────────────────
    // Static Files (Desktop UI)
    // ────────────────────────────────────────────────────────
    let filePath = path.join(UI_DIR, url.pathname === '/' ? 'index.html' : url.pathname);
    const ext = path.extname(filePath);

    fs.readFile(filePath, (err, content) => {
        if (err) {
            res.writeHead(404, { 'Content-Type': 'text/plain' });
            res.end('Not Found');
        } else {
            res.writeHead(200, { 'Content-Type': MIME_TYPES[ext] || 'text/plain' });
            res.end(content);
        }
    });
});

function launchAppWindow() {
    const edgePath = 'C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe';
    const chromePath = 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe';

    let browserExe = null;
    if (fs.existsSync(edgePath)) browserExe = edgePath;
    else if (fs.existsSync(chromePath)) browserExe = chromePath;

    if (browserExe) {
        console.log(`Launching standalone Windows desktop window...`);
        spawn(browserExe, [`--app=http://127.0.0.1:${PORT}`, '--window-size=1280,820'], {
            detached: true,
            stdio: 'ignore'
        }).unref();
    } else {
        console.log(`Open http://127.0.0.1:${PORT} in your browser.`);
    }
}

// Graceful handler if port is already running
server.on('error', (err) => {
    if (err.code === 'EADDRINUSE') {
        console.log(`\n[INFO] PharmaCare bridge is already running on http://127.0.0.1:${PORT}`);
        console.log(`Opening desktop application window...\n`);
        launchAppWindow();
        setTimeout(() => process.exit(0), 1200);
    } else {
        console.error('[ERROR] Server error:', err.message);
    }
});

// Launch Server & Native Desktop Window
server.listen(PORT, '127.0.0.1', () => {
    console.log(`\n======================================================`);
    console.log(`  PharmaCare Desktop Application Started`);
    console.log(`  C++ Engine Path : ${EXE_PATH}`);
    console.log(`  Local Address   : http://127.0.0.1:${PORT}`);
    console.log(`======================================================\n`);

    launchAppWindow();
});

