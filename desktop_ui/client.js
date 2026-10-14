/**
 * ============================================================
 *  PharmaCare Desktop Application — Client Controller
 *  Communicates directly with main.exe via local desktop bridge
 * ============================================================
 */

// Application State Cache
const AppState = {
    stock: [],
    catalog: [],
    suppliers: [],
    alerts: null,
    reports: null
};

// DOM Elements
const elements = {
    tabs: document.querySelectorAll('.nav-item'),
    panes: document.querySelectorAll('.tab-pane'),
    viewTitle: document.getElementById('view-title'),
    viewSubtitle: document.getElementById('view-subtitle'),
    liveCppCmd: document.getElementById('live-cpp-cmd'),
    liveCppStatus: document.getElementById('live-cpp-status'),
    totalStockBadge: document.getElementById('badge-total-stock'),
    toastContainer: document.getElementById('toast-container')
};

// Tab Titles and Subtitles
const TAB_METADATA = {
    'dashboard': {
        title: 'Dashboard Overview',
        sub: 'Live pharmacy metrics processed directly by compiled C++ classes'
    },
    'inventory': {
        title: 'Physical Stock Ledger',
        sub: 'Shelf batches managed by C++ StockItem instances (pharma_inventory.txt)'
    },
    'catalog': {
        title: 'Medicine Catalog',
        sub: 'Polymorphic hierarchy: Tablet, Syrup, and Injection (pharma_medicines.txt)'
    },
    'dispense': {
        title: 'Dispense Medicine',
        sub: 'Executes main.exe --api-sell with C++ isExpired() and safety gates'
    },
    'receive': {
        title: 'Receive Stock',
        sub: 'Executes main.exe --api-receive to restock inventory & log transactions'
    },
    'suppliers': {
        title: 'Verified Suppliers',
        sub: 'Supplier entities inheriting from C++ Person class (pharma_suppliers.txt)'
    },
    'reports': {
        title: 'Reports & Audits',
        sub: 'Financial ledgers and safety audit logs calculated by C++ Inventory engine'
    },
    'oop-guide': {
        title: 'OOP Architecture Guide',
        sub: 'Mapping desktop operations to Object-Oriented Programming concepts in C++'
    }
};

// ────────────────────────────────────────────────────────
// Helper: C++ API Request Handler
// ────────────────────────────────────────────────────────
async function callCppApi(endpoint, method = 'GET', payload = null) {
    try {
        const options = {
            method,
            headers: { 'Content-Type': 'application/json' }
        };
        if (payload) {
            options.body = JSON.stringify(payload);
        }

        const res = await fetch(endpoint, options);
        const json = await res.json();

        // Update live C++ execution footer bar
        if (json.command) {
            elements.liveCppCmd.textContent = json.command;
            elements.liveCppStatus.textContent = json.success ? 'Exit Code: 0 (Success)' : 'Exit Code: 1 (Error)';
            elements.liveCppStatus.style.color = json.success ? '#10b981' : '#ef4444';
        }

        return json;
    } catch (err) {
        elements.liveCppCmd.textContent = `Error connecting to C++ engine bridge: ${err.message}`;
        elements.liveCppStatus.textContent = 'Connection Error';
        elements.liveCppStatus.style.color = '#ef4444';
        showToast('Error communicating with C++ desktop bridge', 'error');
        return { success: false, error: err.message };
    }
}

// ────────────────────────────────────────────────────────
// UI Toast Notifications
// ────────────────────────────────────────────────────────
function showToast(message, type = 'info') {
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    toast.textContent = message;
    elements.toastContainer.appendChild(toast);
    setTimeout(() => {
        toast.style.opacity = '0';
        toast.style.transform = 'translateX(50px)';
        toast.style.transition = 'all 0.3s ease';
        setTimeout(() => toast.remove(), 300);
    }, 3500);
}

// ────────────────────────────────────────────────────────
// Navigation System
// ────────────────────────────────────────────────────────
function switchTab(tabId) {
    elements.tabs.forEach(tab => {
        if (tab.dataset.tab === tabId) {
            tab.classList.add('active');
        } else {
            tab.classList.remove('active');
        }
    });

    elements.panes.forEach(pane => {
        if (pane.id === `pane-${tabId}`) {
            pane.classList.add('active');
        } else {
            pane.classList.remove('active');
        }
    });

    const meta = TAB_METADATA[tabId] || { title: 'PharmaCare', sub: '' };
    elements.viewTitle.textContent = meta.title;
    elements.viewSubtitle.textContent = meta.sub;

    if (tabId === 'dispense') {
        populateDispenseDropdown();
    } else if (tabId === 'receive') {
        populateReceiveDropdowns();
    } else if (tabId === 'reports') {
        loadReportsData();
    }
}

// ────────────────────────────────────────────────────────
// Data Loaders
// ────────────────────────────────────────────────────────
async function loadAllData() {
    await Promise.all([
        loadStockData(),
        loadCatalogData(),
        loadSuppliersData(),
        loadReportsData()
    ]);
    renderDashboard();
    renderInventory();
    renderCatalog();
    renderSuppliers();
    renderReports();
}

async function loadStockData() {
    const res = await callCppApi('/api/stock');
    if (res.success && Array.isArray(res.data)) {
        AppState.stock = res.data.map(item => ({
            ...item,
            name: item.medicineName || item.name,
            batchNumber: item.batch || item.batchNumber,
            unitPrice: item.price !== undefined ? item.price : item.unitPrice,
            expiryDate: item.expiry || item.expiryDate,
            status: item.isExpired ? 'EXPIRED' : (item.isExpiringSoon ? 'EXPIRING SOON' : (item.isLowStock ? 'LOW STOCK' : 'OK'))
        }));
        elements.totalStockBadge.textContent = AppState.stock.length;
    }
}

async function loadCatalogData() {
    const res = await callCppApi('/api/catalog');
    if (res.success && Array.isArray(res.data)) {
        AppState.catalog = res.data.map(med => ({
            ...med,
            basePrice: med.price !== undefined ? med.price : med.basePrice,
            usageInstructions: med.usage || med.usageInstructions
        }));
    }
}

async function loadSuppliersData() {
    const res = await callCppApi('/api/suppliers');
    if (res.success && Array.isArray(res.data)) {
        AppState.suppliers = res.data.map(sup => ({
            ...sup,
            contactName: sup.name || sup.contactName
        }));
    }
}

async function loadReportsData() {
    const res = await callCppApi('/api/reports');
    if (res.success && res.data) {
        AppState.reports = res.data;
        renderReports();
    }
}

// ────────────────────────────────────────────────────────
// 1. DASHBOARD VIEW RENDERING
// ────────────────────────────────────────────────────────
function renderDashboard() {
    // Stat Metrics Calculation
    const totalCatalog = AppState.catalog.length;
    const totalBatches = AppState.stock.length;
    const totalUnits = AppState.stock.reduce((sum, item) => sum + (item.quantity || 0), 0);

    let expiredCount = 0;
    let expiringCount = 0;

    AppState.stock.forEach(item => {
        if (item.status === 'EXPIRED') expiredCount++;
        else if (item.status === 'EXPIRING SOON') expiringCount++;
    });

    document.getElementById('stat-catalog-count').textContent = totalCatalog;
    document.getElementById('stat-batch-count').textContent = totalBatches;
    document.getElementById('stat-units-count').textContent = `${totalUnits} units in stock`;
    document.getElementById('stat-expiring-count').textContent = expiringCount;
    document.getElementById('stat-expired-count').textContent = expiredCount;

    // Formulation hierarchy count
    let tablets = 0, syrups = 0, injections = 0;
    AppState.catalog.forEach(med => {
        if (med.type === 'Tablet') tablets++;
        else if (med.type === 'Syrup') syrups++;
        else if (med.type === 'Injection') injections++;
    });
    document.getElementById('count-tablets').textContent = `${tablets} formulations`;
    document.getElementById('count-syrups').textContent = `${syrups} formulations`;
    document.getElementById('count-injections').textContent = `${injections} formulations`;

    // Critical Batches Table (Expired or Expiring or Low Stock)
    const alertBody = document.getElementById('tbody-dash-alerts');
    alertBody.innerHTML = '';

    const criticalItems = AppState.stock.filter(item => 
        item.status === 'EXPIRED' || item.status === 'EXPIRING SOON' || item.status === 'LOW STOCK'
    );

    if (criticalItems.length === 0) {
        alertBody.innerHTML = `<tr><td colspan="5" style="text-align:center; color:var(--text-muted); padding:1.5rem;">All batches in optimal condition. C++ safety checks passed.</td></tr>`;
    } else {
        criticalItems.forEach(item => {
            const tr = document.createElement('tr');
            let badgeClass = 'badge-safe';
            if (item.status === 'EXPIRED') badgeClass = 'badge-danger';
            else if (item.status === 'EXPIRING SOON') badgeClass = 'badge-warning';
            else if (item.status === 'LOW STOCK') badgeClass = 'badge-info';

            tr.innerHTML = `
                <td><code>${item.stockID}</code></td>
                <td><strong>${item.name}</strong></td>
                <td>${item.batchNumber}</td>
                <td>${item.expiryDate}</td>
                <td><span class="badge ${badgeClass}">${item.status}</span></td>
            `;
            alertBody.appendChild(tr);
        });
    }

    // Load initial sales summary for revenue stat
    callCppApi('/api/reports').then(res => {
        if (res.success && res.data) {
            AppState.reports = res.data;
            const rev = res.data.totalRevenue || 0;
            const salesCount = res.data.sales ? res.data.sales.length : 0;
            document.getElementById('stat-revenue').textContent = `Rs. ${rev.toFixed(2)}`;
            document.getElementById('stat-sales-count').textContent = `${salesCount} transactions`;
        }
    });
}

// ────────────────────────────────────────────────────────
// 2. INVENTORY / STOCK LEDGER RENDERING
// ────────────────────────────────────────────────────────
function renderInventory() {
    const tbody = document.getElementById('tbody-inventory');
    const searchTerm = document.getElementById('input-search-stock').value.toLowerCase().trim();
    const filterStatus = document.getElementById('filter-stock-status').value;

    tbody.innerHTML = '';

    const filtered = AppState.stock.filter(item => {
        const matchesSearch = 
            (item.stockID && item.stockID.toLowerCase().includes(searchTerm)) ||
            (item.name && item.name.toLowerCase().includes(searchTerm)) ||
            (item.batchNumber && item.batchNumber.toLowerCase().includes(searchTerm)) ||
            (item.supplierID && item.supplierID.toLowerCase().includes(searchTerm));

        if (!matchesSearch) return false;

        if (filterStatus === 'ALL') return true;
        if (filterStatus === 'OK' && item.status === 'OK') return true;
        if (filterStatus === 'EXPIRING' && item.status === 'EXPIRING SOON') return true;
        if (filterStatus === 'LOW' && item.status === 'LOW STOCK') return true;
        if (filterStatus === 'EXPIRED' && item.status === 'EXPIRED') return true;
        return false;
    });

    if (filtered.length === 0) {
        tbody.innerHTML = `<tr><td colspan="10" style="text-align:center; color:var(--text-muted); padding:2rem;">No matching stock batches found in pharma_inventory.txt</td></tr>`;
        return;
    }

    filtered.forEach(item => {
        const tr = document.createElement('tr');

        let statusBadge = 'badge-safe';
        if (item.status === 'EXPIRED') statusBadge = 'badge-danger';
        else if (item.status === 'EXPIRING SOON') statusBadge = 'badge-warning';
        else if (item.status === 'LOW STOCK') statusBadge = 'badge-info';

        let typeBadge = 'badge-tablet';
        if (item.type === 'Syrup') typeBadge = 'badge-syrup';
        else if (item.type === 'Injection') typeBadge = 'badge-injection';

        const isActionDisabled = item.status === 'EXPIRED' || item.quantity <= 0;

        tr.innerHTML = `
            <td><code>${item.stockID}</code></td>
            <td><strong>${item.name}</strong></td>
            <td><span class="badge ${typeBadge}">${item.type}</span></td>
            <td>${item.supplierID}</td>
            <td><code>${item.batchNumber}</code></td>
            <td><strong>${item.quantity}</strong> units</td>
            <td>Rs. ${(item.unitPrice || 0).toFixed(2)}</td>
            <td>${item.expiryDate}</td>
            <td><span class="badge ${statusBadge}">${item.status}</span></td>
            <td>
                <button class="btn btn-sm ${isActionDisabled ? 'btn-secondary' : 'btn-primary'} btn-table-dispense" 
                    data-id="${item.stockID}" ${isActionDisabled ? 'disabled title="Blocked by C++ safety check"' : ''}>
                    ${item.status === 'EXPIRED' ? 'Locked (Expired)' : 'Dispense'}
                </button>
            </td>
        `;
        tbody.appendChild(tr);
    });

    // Attach dispense click events
    tbody.querySelectorAll('.btn-table-dispense').forEach(btn => {
        btn.addEventListener('click', () => {
            const stockId = btn.dataset.id;
            switchTab('dispense');
            const select = document.getElementById('dispense-stock-select');
            select.value = stockId;
            select.dispatchEvent(new Event('change'));
        });
    });
}

// ────────────────────────────────────────────────────────
// 3. MEDICINE CATALOG RENDERING
// ────────────────────────────────────────────────────────
function renderCatalog() {
    const grid = document.getElementById('catalog-card-grid');
    const search = document.getElementById('input-search-catalog').value.toLowerCase().trim();

    grid.innerHTML = '';

    const filtered = AppState.catalog.filter(med => 
        (med.id && med.id.toLowerCase().includes(search)) ||
        (med.name && med.name.toLowerCase().includes(search)) ||
        (med.manufacturer && med.manufacturer.toLowerCase().includes(search))
    );

    if (filtered.length === 0) {
        grid.innerHTML = `<div style="grid-column: 1/-1; text-align: center; color: var(--text-muted); padding: 2rem;">No medicines found matching criteria.</div>`;
        return;
    }

    filtered.forEach(med => {
        let typeBadge = 'badge-tablet';
        if (med.type === 'Syrup') typeBadge = 'badge-syrup';
        else if (med.type === 'Injection') typeBadge = 'badge-injection';

        const card = document.createElement('div');
        card.className = 'catalog-card';
        card.innerHTML = `
            <div>
                <div class="catalog-card-header">
                    <div>
                        <h4 class="catalog-card-title">${med.name}</h4>
                        <span class="catalog-card-mfr">${med.manufacturer}</span>
                    </div>
                    <span class="badge ${typeBadge}">${med.type}</span>
                </div>
                <div class="catalog-card-body">
                    <div class="catalog-card-detail"><span>Medicine ID:</span><code>${med.id}</code></div>
                    <div style="margin-top:0.5rem; padding:0.5rem 0.65rem; background:var(--bg-surface-subtle); border-radius:6px; font-size:0.78rem; color:var(--text-secondary); line-height:1.4;">
                        <strong style="color:var(--text-primary); display:block; margin-bottom:0.15rem;">C++ Virtual Instruction:</strong>
                        ${med.usage || med.usageInstructions || 'Standard dosage directions'}
                    </div>
                </div>
            </div>
            <div class="catalog-card-footer">
                <span class="catalog-price">Rs. ${(med.basePrice || 0).toFixed(2)}</span>
                <span style="font-size:0.75rem; color:var(--text-muted);">Base Catalog</span>
            </div>
        `;
        grid.appendChild(card);
    });
}

// ────────────────────────────────────────────────────────
// 4. DISPENSE MEDICINE CONTROLLER
// ────────────────────────────────────────────────────────
function populateDispenseDropdown() {
    const select = document.getElementById('dispense-stock-select');
    select.innerHTML = '<option value="">-- Choose Stock Batch --</option>';

    AppState.stock.forEach(item => {
        const option = document.createElement('option');
        option.value = item.stockID;
        option.textContent = `${item.stockID} — ${item.name} (Batch: ${item.batchNumber}, Qty: ${item.quantity}, Exp: ${item.expiryDate})`;
        if (item.status === 'EXPIRED') {
            option.textContent += ' [EXPIRED - BLOCKED]';
        }
        select.appendChild(option);
    });
}

function updateDispenseInspector() {
    const select = document.getElementById('dispense-stock-select');
    const inspector = document.getElementById('inspector-details');
    const safetyNotice = document.getElementById('dispense-safety-notice');
    const safetyText = document.getElementById('dispense-safety-text');
    const pricePreview = document.getElementById('dispense-total-preview');
    const qtyInput = document.getElementById('dispense-quantity');
    const submitBtn = document.getElementById('btn-complete-sale');

    const stockID = select.value;
    if (!stockID) {
        inspector.innerHTML = `<div class="inspector-placeholder"><span>Select a batch on the left to inspect specifications</span></div>`;
        safetyNotice.style.display = 'none';
        pricePreview.textContent = 'Rs. 0.00';
        submitBtn.disabled = false;
        return;
    }

    const item = AppState.stock.find(s => s.stockID === stockID);
    if (!item) return;

    // Calculate total price preview
    const qty = parseInt(qtyInput.value) || 1;
    const totalPrice = (item.unitPrice || 0) * qty;
    pricePreview.textContent = `Rs. ${totalPrice.toFixed(2)}`;

    // Safety checks
    if (item.status === 'EXPIRED') {
        safetyNotice.style.display = 'block';
        safetyNotice.className = 'safety-box danger';
        safetyText.innerHTML = `<strong>PATIENT SAFETY GATE:</strong> Batch is EXPIRED (${item.expiryDate}). C++ method <code>isExpired()</code> will block execution and reject this transaction.`;
        submitBtn.disabled = true;
    } else if (item.status === 'EXPIRING SOON') {
        safetyNotice.style.display = 'block';
        safetyNotice.className = 'safety-box warning';
        safetyText.innerHTML = `<strong>EXPIRING SOON:</strong> Batch expires within 3 months (${item.expiryDate}). Prioritize dispensing per FEFO protocol.`;
        submitBtn.disabled = false;
    } else if (item.quantity <= 0) {
        safetyNotice.style.display = 'block';
        safetyNotice.className = 'safety-box danger';
        safetyText.innerHTML = `<strong>OUT OF STOCK:</strong> Quantity is 0. C++ engine will reject sale with insufficient stock error.`;
        submitBtn.disabled = true;
    } else {
        safetyNotice.style.display = 'block';
        safetyNotice.className = 'safety-box safe';
        safetyText.innerHTML = `<strong>BATCH VERIFIED:</strong> Batch ${item.batchNumber} is active, within safe expiry, and ready to dispense.`;
        submitBtn.disabled = false;
    }

    // Inspector Details View
    inspector.innerHTML = `
        <table class="inspector-table">
            <tr><td>Stock ID:</td><td><code>${item.stockID}</code></td></tr>
            <tr><td>Medicine ID:</td><td><code>${item.medicineID}</code></td></tr>
            <tr><td>Medicine Name:</td><td><strong>${item.name}</strong></td></tr>
            <tr><td>Formulation:</td><td>${item.type}</td></tr>
            <tr><td>Batch Number:</td><td><code>${item.batchNumber}</code></td></tr>
            <tr><td>Shelf Quantity:</td><td>${item.quantity} units</td></tr>
            <tr><td>Unit Price:</td><td>Rs. ${(item.unitPrice || 0).toFixed(2)}</td></tr>
            <tr><td>Expiry Date:</td><td>${item.expiryDate}</td></tr>
            <tr><td>Supplier:</td><td>${item.supplierID}</td></tr>
            <tr><td>Safety Status:</td><td><strong>${item.status}</strong></td></tr>
        </table>
    `;
}

// ────────────────────────────────────────────────────────
// 5. RECEIVE STOCK CONTROLLER
// ────────────────────────────────────────────────────────
function populateReceiveDropdowns() {
    const medSelect = document.getElementById('receive-medicine-select');
    const supSelect = document.getElementById('receive-supplier-select');

    medSelect.innerHTML = '<option value="">-- Choose Medicine --</option>';
    AppState.catalog.forEach(med => {
        const opt = document.createElement('option');
        opt.value = med.id;
        opt.textContent = `${med.id} — ${med.name} (${med.type})`;
        medSelect.appendChild(opt);
    });

    supSelect.innerHTML = '<option value="">-- Choose Supplier --</option>';
    AppState.suppliers.forEach(sup => {
        const opt = document.createElement('option');
        opt.value = sup.id;
        opt.textContent = `${sup.id} — ${sup.company} (${sup.contactName})`;
        supSelect.appendChild(opt);
    });
}

// ────────────────────────────────────────────────────────
// 6. SUPPLIERS RENDERING
// ────────────────────────────────────────────────────────
function renderSuppliers() {
    const grid = document.getElementById('suppliers-card-grid');
    grid.innerHTML = '';

    if (AppState.suppliers.length === 0) {
        grid.innerHTML = `<div style="grid-column: 1/-1; text-align: center; color: var(--text-muted); padding: 2rem;">No verified suppliers loaded.</div>`;
        return;
    }

    AppState.suppliers.forEach(sup => {
        const card = document.createElement('div');
        card.className = 'supplier-card';
        card.innerHTML = `
            <h4>${sup.company}</h4>
            <div class="supplier-meta">Supplier ID: <code>${sup.id}</code> &bull; Verified Partner</div>
            <div class="supplier-row">
                <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/></svg>
                <span><strong>${sup.contactName}</strong> (Representative)</span>
            </div>
            <div class="supplier-row">
                <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M22 16.92v3a2 2 0 0 1-2.18 2 19.79 19.79 0 0 1-8.63-3.07 19.5 19.5 0 0 1-6-6 19.79 19.79 0 0 1-3.07-8.67A2 2 0 0 1 4.11 2h3a2 2 0 0 1 2 1.72 12.84 12.84 0 0 0 .7 2.81 2 2 0 0 1-.45 2.11L8.09 9.91a16 16 0 0 0 6 6l1.27-1.27a2 2 0 0 1 2.11-.45 12.84 12.84 0 0 0 2.81.7A2 2 0 0 1 22 16.92z"/></svg>
                <span>${sup.phone}</span>
            </div>
            <div class="supplier-row">
                <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 4h16c1.1 0 2 .9 2 2v12c0 1.1-.9 2-2 2H4c-1.1 0-2-.9-2-2V6c0-1.1.9-2 2-2z"/><polyline points="22,6 12,13 2,6"/></svg>
                <span>${sup.email || 'N/A'}</span>
            </div>
            <div class="supplier-row">
                <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 10c0 7-9 13-9 13s-9-6-9-13a9 9 0 0 1 18 0z"/><circle cx="12" cy="10" r="3"/></svg>
                <span>${sup.address || 'N/A'}</span>
            </div>
        `;
        grid.appendChild(card);
    });
}

// ────────────────────────────────────────────────────────
// 7. REPORTS & AUDITS RENDERING
// ────────────────────────────────────────────────────────
function renderReports() {
    // 1. Expiry Audit (From C++ stock state or reports)
    const tbodyExpiry = document.getElementById('tbody-report-expiry');
    tbodyExpiry.innerHTML = '';
    const expiryAudit = (AppState.reports && AppState.reports.expiryAudit && AppState.reports.expiryAudit.length > 0)
        ? AppState.reports.expiryAudit
        : AppState.stock.filter(item => item.status === 'EXPIRED' || item.status === 'EXPIRING SOON');

    if (expiryAudit.length === 0) {
        tbodyExpiry.innerHTML = `<tr><td colspan="5" style="text-align:center; padding:1.25rem; color:var(--text-muted);">No expiring batches found. All batches pass C++ isExpired() safety check.</td></tr>`;
    } else {
        expiryAudit.forEach(item => {
            const tr = document.createElement('tr');
            const badgeClass = item.status === 'EXPIRED' ? 'badge-danger' : 'badge-warning';
            tr.innerHTML = `
                <td><code>${item.stockID}</code></td>
                <td>${item.name || item.medicineName}</td>
                <td>${item.batchNumber || item.batch}</td>
                <td>${item.expiryDate || item.expiry}</td>
                <td><span class="badge ${badgeClass}">${item.status}</span></td>
            `;
            tbodyExpiry.appendChild(tr);
        });
    }

    // 2. Low Stock Triggers (From C++ stock state or reports)
    const tbodyLowStock = document.getElementById('tbody-report-lowstock');
    tbodyLowStock.innerHTML = '';
    const lowStockAudit = (AppState.reports && AppState.reports.lowStockAudit && AppState.reports.lowStockAudit.length > 0)
        ? AppState.reports.lowStockAudit
        : AppState.stock.filter(item => item.status === 'LOW STOCK' || (item.quantity !== undefined && item.minStockLevel !== undefined && item.quantity <= item.minStockLevel));

    if (lowStockAudit.length === 0) {
        tbodyLowStock.innerHTML = `<tr><td colspan="5" style="text-align:center; padding:1.25rem; color:var(--text-muted);">All shelf items meet minimum stock thresholds.</td></tr>`;
    } else {
        lowStockAudit.forEach(item => {
            const tr = document.createElement('tr');
            tr.innerHTML = `
                <td><code>${item.stockID}</code></td>
                <td>${item.name || item.medicineName}</td>
                <td><strong>${item.quantity}</strong></td>
                <td>${item.minStockLevel || item.threshold || 10}</td>
                <td><span class="badge badge-warning">REORDER</span></td>
            `;
            tbodyLowStock.appendChild(tr);
        });
    }

    // 3. Sales Ledger (pharma_sales.txt)
    const tbodySales = document.getElementById('tbody-report-sales');
    tbodySales.innerHTML = '';
    const sales = (AppState.reports && AppState.reports.sales) ? AppState.reports.sales : [];
    let totalRevenue = (AppState.reports && AppState.reports.totalRevenue !== undefined) ? AppState.reports.totalRevenue : 0;

    if (sales.length === 0) {
        tbodySales.innerHTML = `<tr><td colspan="6" style="text-align:center; padding:1.25rem; color:var(--text-muted);">No sales recorded yet. Dispense medicine to generate transactions.</td></tr>`;
    } else {
        sales.forEach(s => {
            const saleTotal = s.total !== undefined ? s.total : ((s.quantity || s.qty || 0) * (s.price || 0));
            const tr = document.createElement('tr');
            tr.innerHTML = `
                <td><code>${s.saleID || 'N/A'}</code></td>
                <td>${s.medicineName || s.name || 'Unknown'}</td>
                <td>${s.customer || s.customerName || 'Walk-in'}</td>
                <td>${s.quantity !== undefined ? s.quantity : (s.qty || 0)}</td>
                <td>Rs. ${saleTotal.toFixed(2)}</td>
                <td>${s.date || 'N/A'}</td>
            `;
            tbodySales.appendChild(tr);
        });
    }
    document.getElementById('total-revenue-badge').textContent = `Rs. ${totalRevenue.toFixed(2)}`;

    // 4. Purchases Ledger (pharma_purchases.txt)
    const tbodyPurchases = document.getElementById('tbody-report-purchases');
    tbodyPurchases.innerHTML = '';
    const purchases = (AppState.reports && AppState.reports.purchases) ? AppState.reports.purchases : [];
    let totalExpense = (AppState.reports && AppState.reports.totalExpenses !== undefined) ? AppState.reports.totalExpenses : 0;

    if (purchases.length === 0) {
        tbodyPurchases.innerHTML = `<tr><td colspan="6" style="text-align:center; padding:1.25rem; color:var(--text-muted);">No procurement expenses logged yet.</td></tr>`;
    } else {
        purchases.forEach(p => {
            const purchaseCost = p.total !== undefined ? p.total : (p.cost !== undefined ? p.cost : 0);
            const tr = document.createElement('tr');
            tr.innerHTML = `
                <td><code>${p.purchaseID || 'N/A'}</code></td>
                <td>${p.medicineName || p.name || 'Unknown'}</td>
                <td>${p.supplierID || p.supplier || 'N/A'}</td>
                <td>${p.quantity !== undefined ? p.quantity : (p.qty || 0)}</td>
                <td>Rs. ${purchaseCost.toFixed(2)}</td>
                <td>${p.date || 'N/A'}</td>
            `;
            tbodyPurchases.appendChild(tr);
        });
    }
    document.getElementById('total-expense-badge').textContent = `Rs. ${totalExpense.toFixed(2)}`;
}

// ────────────────────────────────────────────────────────
// Modal Form Handlers
// ────────────────────────────────────────────────────────
function setupModals() {
    const modalMed = document.getElementById('modal-add-medicine');
    const modalSup = document.getElementById('modal-add-supplier');
    const medTypeSelect = document.getElementById('modal-med-type');
    const typeFieldsContainer = document.getElementById('modal-type-fields');

    // Dynamic Formulation Fields
    function updateTypeFields() {
        const type = medTypeSelect.value;
        if (type === 'Tablet') {
            typeFieldsContainer.innerHTML = `
                <div class="form-row">
                    <div class="form-group">
                        <label>Dosage (mg) <span class="required">*</span></label>
                        <input type="number" id="modal-med-f1" placeholder="e.g. 500" value="500" required>
                    </div>
                    <div class="form-group">
                        <label>Pack Size (Count) <span class="required">*</span></label>
                        <input type="number" id="modal-med-f2" placeholder="e.g. 10" value="10" required>
                    </div>
                </div>
            `;
        } else if (type === 'Syrup') {
            typeFieldsContainer.innerHTML = `
                <div class="form-row">
                    <div class="form-group">
                        <label>Bottle Volume (ml) <span class="required">*</span></label>
                        <input type="number" id="modal-med-f1" placeholder="e.g. 100" value="100" required>
                    </div>
                    <div class="form-group">
                        <label>Sugar Free? (1 for Yes, 0 for No)</label>
                        <select id="modal-med-f2">
                            <option value="1">1 (Sugar Free)</option>
                            <option value="0">0 (Regular / Sugar)</option>
                        </select>
                    </div>
                </div>
            `;
        } else if (type === 'Injection') {
            typeFieldsContainer.innerHTML = `
                <div class="form-row">
                    <div class="form-group">
                        <label>Vial Volume (ml) <span class="required">*</span></label>
                        <input type="number" id="modal-med-f1" placeholder="e.g. 2" value="2" required>
                    </div>
                    <div class="form-group">
                        <label>Administration Route</label>
                        <input type="text" id="modal-med-f2" placeholder="e.g. IV / IM" value="IV/IM" required>
                    </div>
                </div>
            `;
        }
    }

    medTypeSelect.addEventListener('change', updateTypeFields);
    updateTypeFields();

    // Open/Close buttons
    document.getElementById('btn-add-medicine-modal').addEventListener('click', () => {
        modalMed.classList.add('active');
    });
    const btnCatalogAdd = document.getElementById('btn-add-med-catalog');
    if (btnCatalogAdd) {
        btnCatalogAdd.addEventListener('click', () => modalMed.classList.add('active'));
    }

    document.getElementById('btn-close-med-modal').addEventListener('click', () => {
        modalMed.classList.remove('active');
    });
    document.getElementById('btn-cancel-med-modal').addEventListener('click', () => {
        modalMed.classList.remove('active');
    });

    document.getElementById('btn-add-supplier-modal').addEventListener('click', () => {
        modalSup.classList.add('active');
    });
    document.getElementById('btn-close-sup-modal').addEventListener('click', () => {
        modalSup.classList.remove('active');
    });
    document.getElementById('btn-cancel-sup-modal').addEventListener('click', () => {
        modalSup.classList.remove('active');
    });

    // Submit Medicine Modal Form
    document.getElementById('form-modal-medicine').addEventListener('submit', async (e) => {
        e.preventDefault();
        const payload = {
            type: medTypeSelect.value,
            id: document.getElementById('modal-med-id').value.trim(),
            name: document.getElementById('modal-med-name').value.trim(),
            manufacturer: document.getElementById('modal-med-mfr').value.trim(),
            price: parseFloat(document.getElementById('modal-med-price').value),
            field1: document.getElementById('modal-med-f1').value.trim(),
            field2: document.getElementById('modal-med-f2').value.trim()
        };

        const res = await callCppApi('/api/add-medicine', 'POST', payload);
        if (res.success && res.data && res.data.success) {
            showToast(`Medicine ${payload.id} saved to C++ catalog!`, 'success');
            modalMed.classList.remove('active');
            e.target.reset();
            updateTypeFields();
            await loadCatalogData();
            renderCatalog();
            renderDashboard();
        } else {
            showToast(res.data?.error || res.error || 'Failed to save medicine', 'error');
        }
    });

    // Submit Supplier Modal Form
    document.getElementById('form-modal-supplier').addEventListener('submit', async (e) => {
        e.preventDefault();
        const payload = {
            id: document.getElementById('modal-sup-id').value.trim(),
            company: document.getElementById('modal-sup-company').value.trim(),
            name: document.getElementById('modal-sup-name').value.trim(),
            phone: document.getElementById('modal-sup-phone').value.trim(),
            email: document.getElementById('modal-sup-email').value.trim() || 'N/A',
            address: document.getElementById('modal-sup-address').value.trim() || 'N/A'
        };

        const res = await callCppApi('/api/add-supplier', 'POST', payload);
        if (res.success && res.data && res.data.success) {
            showToast(`Supplier ${payload.id} saved to C++ registry!`, 'success');
            modalSup.classList.remove('active');
            e.target.reset();
            await loadSuppliersData();
            renderSuppliers();
        } else {
            showToast(res.data?.error || res.error || 'Failed to save supplier', 'error');
        }
    });
}

// ────────────────────────────────────────────────────────
// Form Event Listeners (Dispense & Receive)
// ────────────────────────────────────────────────────────
function setupFormListeners() {
    // 1. Dispense Form
    const dispenseSelect = document.getElementById('dispense-stock-select');
    const dispenseQty = document.getElementById('dispense-quantity');

    dispenseSelect.addEventListener('change', updateDispenseInspector);
    dispenseQty.addEventListener('input', updateDispenseInspector);

    document.getElementById('form-dispense').addEventListener('submit', async (e) => {
        e.preventDefault();
        const stockID = dispenseSelect.value;
        const quantity = parseInt(dispenseQty.value);
        const customerName = document.getElementById('dispense-customer-name').value.trim();

        if (!stockID) {
            showToast('Please select a stock batch', 'error');
            return;
        }

        const res = await callCppApi('/api/sell', 'POST', { stockID, quantity, customerName });

        if (res.success && res.data && res.data.success) {
            showToast(`C++ Engine Executed Sale: Rs. ${res.data.totalPrice.toFixed(2)} charged. Remaining: ${res.data.remainingQty}`, 'success');
            document.getElementById('form-dispense').reset();
            updateDispenseInspector();
            await loadAllData();
        } else {
            const err = res.data?.error || res.error || 'Sale failed';
            showToast(`C++ SAFETY REJECTION: ${err}`, 'error');
        }
    });

    // 2. Receive Form
    document.getElementById('form-receive').addEventListener('submit', async (e) => {
        e.preventDefault();
        const payload = {
            medicineID: document.getElementById('receive-medicine-select').value,
            supplierID: document.getElementById('receive-supplier-select').value,
            batchNumber: document.getElementById('receive-batch-number').value.trim(),
            quantity: parseInt(document.getElementById('receive-quantity').value),
            cost: parseFloat(document.getElementById('receive-cost').value),
            minStock: parseInt(document.getElementById('receive-min-stock').value),
            mfgDate: document.getElementById('receive-mfg-date').value.trim(),
            expiryDate: document.getElementById('receive-expiry-date').value.trim()
        };

        const res = await callCppApi('/api/receive', 'POST', payload);

        if (res.success && res.data && res.data.success) {
            showToast(`C++ Engine Restocked: Batch ${payload.batchNumber} added with Stock ID ${res.data.stockID}`, 'success');
            document.getElementById('form-receive').reset();
            await loadAllData();
            switchTab('inventory');
        } else {
            const err = res.data?.error || res.error || 'Failed to receive stock';
            showToast(`C++ Restock Rejection: ${err}`, 'error');
        }
    });

    // Quick Action Buttons
    document.getElementById('btn-quick-dispense').addEventListener('click', () => switchTab('dispense'));
    document.getElementById('btn-add-batch-modal').addEventListener('click', () => switchTab('receive'));
    document.getElementById('link-goto-inventory').addEventListener('click', () => switchTab('inventory'));

    // Search & Filter Listeners
    document.getElementById('input-search-stock').addEventListener('input', renderInventory);
    document.getElementById('filter-stock-status').addEventListener('change', renderInventory);
    document.getElementById('input-search-catalog').addEventListener('input', renderCatalog);
}

// ────────────────────────────────────────────────────────
// Application Initialization
// ────────────────────────────────────────────────────────
window.addEventListener('DOMContentLoaded', async () => {
    // Tab switching event delegation
    elements.tabs.forEach(tab => {
        tab.addEventListener('click', () => {
            switchTab(tab.dataset.tab);
        });
    });

    setupModals();
    setupFormListeners();

    // Initial data fetch from C++ binary
    await loadAllData();
});
