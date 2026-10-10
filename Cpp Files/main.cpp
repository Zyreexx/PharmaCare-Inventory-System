// ============================================================
//  main.cpp
//  Pharma Inventory Management System
//  Modular 3-File Architecture:
//    1. pharma_medicines.txt  -- Product Master Catalog
//    2. pharma_inventory.txt  -- Physical Stock & Batch Ledger
//    3. pharma_suppliers.txt  -- Supplier Registry
//
//  OOP Concepts Demonstrated:
//  --------------------------
//  Classes & Objects     : Medicine, StockItem, Supplier, Customer, Sale, Purchase, Alert, Inventory
//  Encapsulation         : Data hiding via private/protected fields, validated getters/setters
//  Inheritance           : Person -> Supplier, Customer
//                          Medicine -> Tablet, Syrup, Injection
//                          Alert -> ExpiryAlert, LowStockAlert
//  Polymorphism          : vector<Medicine*>, vector<Alert*>, vector<Supplier*>
//                          with base-class pointers processing derived objects at runtime
//  Virtual Functions     : display(), getType(), getUsageInstructions(),
//                          getAlertType(), getRole(), serialize()
//  Pure Virtual (= 0)    : Medicine::getType(), Medicine::getUsageInstructions(),
//                          Alert::getAlertType(), Person::getRole()
//  Virtual Destructors   : virtual ~Medicine(), virtual ~Person(), virtual ~Alert()
//  Pointers & Memory     : Dynamic allocation with new/delete; cleaned up in Inventory::~Inventory()
//  File Handling         : Normalised 3-File database (medicines, inventory, suppliers)
//                          plus audit logs (pharma_sales.txt, pharma_purchases.txt, pharma_alerts.txt)
// ============================================================

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>

using namespace std;
 
// ============================================================
//  ANSI Colour Namespace (ASCII-safe terminal output)
// ============================================================
namespace Color {
    const string RESET   = "\033[0m";
    const string RED     = "\033[1;31m";
    const string GREEN   = "\033[1;32m";
    const string YELLOW  = "\033[1;33m";
    const string BLUE    = "\033[1;34m";
    const string CYAN    = "\033[1;36m";
    const string MAGENTA = "\033[1;35m";
    const string WHITE   = "\033[1;37m";
    const string BOLD    = "\033[1m";
}

// ============================================================
//  Utility Helpers
// ============================================================
void printBanner(const string& title) {
    cout << "\n" << Color::CYAN << string(64, '=') << Color::RESET << "\n";
    int pad = (64 - (int)title.size()) / 2;
    cout << Color::BOLD << string(pad > 0 ? pad : 0, ' ')
         << title << Color::RESET << "\n";
    cout << Color::CYAN << string(64, '=') << Color::RESET << "\n";
}

void printSection(const string& s) {
    cout << "\n" << Color::BLUE << "--- " << s << " ---" << Color::RESET << "\n";
}

void printOK(const string& s)   { cout << Color::GREEN  << "  [OK]  " << s << Color::RESET << "\n"; }
void printWarn(const string& s) { cout << Color::YELLOW << "  [!!]  " << s << Color::RESET << "\n"; }
void printErr(const string& s)  { cout << Color::RED    << "  [X]   " << s << Color::RESET << "\n"; }
void printInfo(const string& s) { cout << Color::WHITE  << "  [i]   " << s << Color::RESET << "\n"; }

string currentDate() {
    time_t t = time(nullptr);
    tm* ti = localtime(&t);
    char buf[12];
    strftime(buf, sizeof(buf), "%d/%m/%Y", ti);
    return string(buf);
}

int currentMonth() { time_t t = time(nullptr); return localtime(&t)->tm_mon + 1;     }
int currentYear()  { time_t t = time(nullptr); return localtime(&t)->tm_year + 1900; }

string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    return (a == string::npos) ? "" : s.substr(a, b - a + 1);
}

vector<string> split(const string& s, char d) {
    vector<string> out;
    string tok;
    istringstream ss(s);
    while (getline(ss, tok, d)) out.push_back(trim(tok));
    return out;
}

// ============================================================
//  ExpiryDate — stores MM/YYYY and comparison helpers
// ============================================================
struct ExpiryDate {
    int month, year;

    ExpiryDate(int m = 1, int y = 2025) : month(m), year(y) {}

    static ExpiryDate parse(const string& s) {
        vector<string> p = split(s, '/');
        if (p.size() == 2) {
            try { return ExpiryDate(stoi(p[0]), stoi(p[1])); }
            catch (...) {}
        }
        return ExpiryDate(1, 2025);
    }

    string toString() const {
        ostringstream ss;
        ss << setw(2) << setfill('0') << month << "/" << year;
        return ss.str();
    }

    bool isExpired() const {
        int cm = currentMonth(), cy = currentYear();
        return (year < cy) || (year == cy && month <= cm);
    }

    bool isExpiringSoon() const {
        if (isExpired()) return false;
        int total_now    = currentYear() * 12 + currentMonth();
        int total_expiry = year          * 12 + month;
        return (total_expiry - total_now) <= 3;
    }
};

// ============================================================
//  BASE CLASS: Person
//  DERIVED:    Supplier, Customer
// ============================================================
class Person {
protected:
    string m_id, m_name, m_phone, m_address;

public:
    Person(const string& id, const string& name,
           const string& phone, const string& address)
        : m_id(id), m_name(name), m_phone(phone), m_address(address) {}

    virtual ~Person() {}

    virtual string getRole() const = 0;  // Pure virtual

    virtual void display() const {
        cout << "  ID      : " << m_id      << "\n";
        cout << "  Name    : " << m_name    << "\n";
        cout << "  Role    : " << getRole() << "\n";
        cout << "  Phone   : " << m_phone   << "\n";
        cout << "  Address : " << m_address << "\n";
    }

    virtual string serialize() const {
        return m_id + "|" + m_name + "|" + m_phone + "|" + m_address;
    }

    string getID()      const { return m_id;      }
    string getName()    const { return m_name;     }
    string getPhone()   const { return m_phone;    }
    string getAddress() const { return m_address;  }
};

// ── Derived: Supplier ────────────────────────────────────────
class Supplier : public Person {
private:
    string m_company;
    string m_email;

public:
    Supplier(const string& id, const string& name, const string& phone,
             const string& address, const string& company, const string& email)
        : Person(id, name, phone, address), m_company(company), m_email(email) {}

    string getRole() const override { return "Supplier"; }

    void display() const override {
        cout << Color::BOLD << "\n  [SUPPLIER]\n" << Color::RESET;
        Person::display();
        cout << "  Company : " << m_company << "\n";
        cout << "  Email   : " << m_email   << "\n";
    }

    string serialize() const override {
        return Person::serialize() + "|" + m_company + "|" + m_email;
    }

    static Supplier* deserialize(const string& line) {
        vector<string> p = split(line, '|');
        if (p.size() >= 6)
            return new Supplier(p[0], p[1], p[2], p[3], p[4], p[5]);
        return nullptr;
    }

    string getCompany() const { return m_company; }
    string getEmail()   const { return m_email;   }
};

// ── Derived: Customer ────────────────────────────────────────
class Customer : public Person {
private:
    string m_prescriptionID;

public:
    Customer(const string& id, const string& name, const string& phone,
             const string& address, const string& rxID = "")
        : Person(id, name, phone, address), m_prescriptionID(rxID) {}

    string getRole() const override { return "Customer"; }

    void display() const override {
        cout << Color::BOLD << "\n  [CUSTOMER]\n" << Color::RESET;
        Person::display();
        if (!m_prescriptionID.empty())
            cout << "  Rx ID   : " << m_prescriptionID << "\n";
    }
};

// ============================================================
//  ABSTRACT BASE CLASS: Medicine (Catalog Master)
//  DERIVED:             Tablet, Syrup, Injection
//  Stored in:           pharma_medicines.txt
// ============================================================
class Medicine {
protected:
    string m_id;
    string m_name;
    string m_manufacturer;
    double m_price;

public:
    Medicine(const string& id, const string& name,
             const string& manufacturer, double price)
        : m_id(id), m_name(name), m_manufacturer(manufacturer), m_price(price) {}

    virtual ~Medicine() {}

    virtual string getType()              const = 0;
    virtual string getUsageInstructions() const = 0;

    virtual void display() const {
        cout << "  ID           : " << Color::BOLD << m_id << Color::RESET << "\n";
        cout << "  Name         : " << Color::BOLD << m_name << Color::RESET << "\n";
        cout << "  Type         : " << getType() << "\n";
        cout << "  Manufacturer : " << m_manufacturer << "\n";
        cout << "  Base Price   : Rs. " << fixed << setprecision(2) << m_price << "\n";
        cout << "  Usage        : " << getUsageInstructions() << "\n";
    }

    virtual string serialize() const {
        ostringstream ss;
        ss << getType() << "|" << m_id << "|" << m_name << "|"
           << m_manufacturer << "|" << fixed << setprecision(2) << m_price;
        return ss.str();
    }

    string getID()           const { return m_id;           }
    string getName()         const { return m_name;         }
    string getManufacturer() const { return m_manufacturer; }
    double getPrice()        const { return m_price;        }

    void setPrice(double p)  { m_price = p; }
};

// ── Derived: Tablet ──────────────────────────────────────────
class Tablet : public Medicine {
private:
    int m_mgPerTablet;
    int m_tabletsPerStrip;

public:
    Tablet(const string& id, const string& name, const string& mfr,
           double price, int mg, int strips)
        : Medicine(id, name, mfr, price),
          m_mgPerTablet(mg), m_tabletsPerStrip(strips) {}

    string getType() const override { return "Tablet"; }

    string getUsageInstructions() const override {
        return "Take " + to_string(m_mgPerTablet) + "mg orally with water. (" +
               to_string(m_tabletsPerStrip) + " tablets/strip).";
    }

    void display() const override {
        Medicine::display();
        cout << "  Strength     : " << m_mgPerTablet << " mg per tablet\n";
        cout << "  Strip Size   : " << m_tabletsPerStrip << " tablets per strip\n";
    }

    string serialize() const override {
        return Medicine::serialize() + "|" + to_string(m_mgPerTablet) + "|" + to_string(m_tabletsPerStrip);
    }
};

// ── Derived: Syrup ───────────────────────────────────────────
class Syrup : public Medicine {
private:
    double m_mlPerBottle;
    string m_flavour;

public:
    Syrup(const string& id, const string& name, const string& mfr,
          double price, double ml, const string& flavour)
        : Medicine(id, name, mfr, price),
          m_mlPerBottle(ml), m_flavour(flavour) {}

    string getType() const override { return "Syrup"; }

    string getUsageInstructions() const override {
        ostringstream ss;
        ss << "Take 5-10ml orally. Bottle: " << fixed << setprecision(0)
           << m_mlPerBottle << "ml. Shake well before use.";
        return ss.str();
    }

    void display() const override {
        Medicine::display();
        cout << "  Volume       : " << m_mlPerBottle << " ml per bottle\n";
        cout << "  Flavour      : " << m_flavour << "\n";
    }

    string serialize() const override {
        ostringstream ss;
        ss << Medicine::serialize() << "|" << fixed << setprecision(1) << m_mlPerBottle << "|" << m_flavour;
        return ss.str();
    }
};

// ── Derived: Injection ───────────────────────────────────────
class Injection : public Medicine {
private:
    double m_mlPerVial;
    string m_route;

public:
    Injection(const string& id, const string& name, const string& mfr,
              double price, double ml, const string& route)
        : Medicine(id, name, mfr, price),
          m_mlPerVial(ml), m_route(route) {}

    string getType() const override { return "Injection"; }

    string getUsageInstructions() const override {
        ostringstream ss;
        ss << "Administer " << fixed << setprecision(0) << m_mlPerVial
           << "ml via " << m_route << " route. Clinical administration only.";
        return ss.str();
    }

    void display() const override {
        Medicine::display();
        cout << "  Vial Volume  : " << m_mlPerVial << " ml\n";
        cout << "  Route        : " << m_route << "\n";
    }

    string serialize() const override {
        ostringstream ss;
        ss << Medicine::serialize() << "|" << fixed << setprecision(1) << m_mlPerVial << "|" << m_route;
        return ss.str();
    }
};

// ============================================================
//  StockItem — Physical Inventory Ledger Record
//  Stored in: pharma_inventory.txt
//  Relational Links: m_medicineID (Medicine), m_supplierID (Supplier)
// ============================================================
class StockItem {
private:
    string     m_stockID;
    string     m_medicineID;
    string     m_supplierID;
    string     m_batchNumber;
    int        m_quantity;
    string     m_mfgDate;
    ExpiryDate m_expiry;
    int        m_minStock;

public:
    StockItem(const string& stockID, const string& medID, const string& supID,
              const string& batch, int qty, const string& mfgDate,
              const ExpiryDate& expiry, int minStock)
        : m_stockID(stockID), m_medicineID(medID), m_supplierID(supID),
          m_batchNumber(batch), m_quantity(qty), m_mfgDate(mfgDate),
          m_expiry(expiry), m_minStock(minStock) {}

    string     getStockID()    const { return m_stockID;     }
    string     getMedicineID() const { return m_medicineID;  }
    string     getSupplierID() const { return m_supplierID;  }
    string     getBatch()      const { return m_batchNumber; }
    int        getQuantity()   const { return m_quantity;    }
    string     getMfgDate()    const { return m_mfgDate;     }
    ExpiryDate getExpiry()     const { return m_expiry;      }
    int        getMinStock()   const { return m_minStock;    }

    void setQuantity(int q) { m_quantity = q; }

    bool isExpired()      const { return m_expiry.isExpired();      }
    bool isExpiringSoon() const { return m_expiry.isExpiringSoon(); }
    bool isLowStock()     const { return m_quantity <= m_minStock;  }

    string serialize() const {
        ostringstream ss;
        ss << m_stockID << "|" << m_medicineID << "|" << m_supplierID << "|"
           << m_batchNumber << "|" << m_quantity << "|" << m_mfgDate << "|"
           << m_expiry.toString() << "|" << m_minStock;
        return ss.str();
    }

    static StockItem* deserialize(const string& line) {
        vector<string> p = split(line, '|');
        if (p.size() >= 8) {
            try {
                return new StockItem(p[0], p[1], p[2], p[3],
                                     stoi(p[4]), p[5],
                                     ExpiryDate::parse(p[6]), stoi(p[7]));
            } catch (...) {}
        }
        return nullptr;
    }

    void display(const Medicine* med = nullptr, const Supplier* sup = nullptr) const {
        cout << "  Stock ID     : " << Color::BOLD << m_stockID << Color::RESET << "\n";
        cout << "  Medicine ID  : " << m_medicineID;
        if (med) cout << " (" << Color::BOLD << med->getName() << Color::RESET << ", " << med->getType() << ")";
        cout << "\n";

        cout << "  Supplier ID  : " << m_supplierID;
        if (sup) cout << " (" << sup->getCompany() << " - " << sup->getName() << ")";
        cout << "\n";

        cout << "  Batch No.    : " << m_batchNumber << "\n";

        cout << "  Quantity     : ";
        if (isLowStock())
            cout << Color::RED << m_quantity << " [LOW STOCK! Threshold: " << m_minStock << "]" << Color::RESET << "\n";
        else
            cout << Color::GREEN << m_quantity << Color::RESET << "\n";

        cout << "  Mfg Date     : " << m_mfgDate << "\n";

        cout << "  Expiry       : ";
        if (isExpired())
            cout << Color::RED << m_expiry.toString() << " [EXPIRED - DO NOT DISPENSE]" << Color::RESET << "\n";
        else if (isExpiringSoon())
            cout << Color::YELLOW << m_expiry.toString() << " [EXPIRING SOON]" << Color::RESET << "\n";
        else
            cout << Color::GREEN << m_expiry.toString() << Color::RESET << "\n";

        cout << "  Min Stock    : " << m_minStock << " units\n";
    }
};

// ============================================================
//  Transaction Structs
// ============================================================
struct Sale {
    string saleID, stockID, medicineID, medicineName, customerName, date;
    int    quantity;
    double priceEach;

    void display() const {
        cout << "  Sale ID      : " << saleID << "\n";
        cout << "  Stock ID     : " << stockID << "\n";
        cout << "  Medicine     : " << medicineName << " (" << medicineID << ")\n";
        cout << "  Customer     : " << customerName << "\n";
        cout << "  Quantity     : " << quantity << "\n";
        cout << "  Price Each   : Rs. " << fixed << setprecision(2) << priceEach << "\n";
        cout << "  Total Amount : Rs. " << fixed << setprecision(2) << (quantity * priceEach) << "\n";
        cout << "  Date         : " << date << "\n";
    }

    string serialize() const {
        ostringstream ss;
        ss << saleID << "|" << stockID << "|" << medicineID << "|"
           << medicineName << "|" << customerName << "|" << quantity << "|"
           << fixed << setprecision(2) << priceEach << "|" << date;
        return ss.str();
    }
};

struct Purchase {
    string purchaseID, stockID, medicineID, medicineName, supplierID, date;
    int    quantity;
    double costEach;

    void display() const {
        cout << "  Purchase ID  : " << purchaseID << "\n";
        cout << "  Stock / Batch: " << stockID << "\n";
        cout << "  Medicine     : " << medicineName << " (" << medicineID << ")\n";
        cout << "  Supplier     : " << supplierID << "\n";
        cout << "  Quantity     : " << quantity << "\n";
        cout << "  Cost Each    : Rs. " << fixed << setprecision(2) << costEach << "\n";
        cout << "  Total Cost   : Rs. " << fixed << setprecision(2) << (quantity * costEach) << "\n";
        cout << "  Date         : " << date << "\n";
    }

    string serialize() const {
        ostringstream ss;
        ss << purchaseID << "|" << stockID << "|" << medicineID << "|"
           << medicineName << "|" << supplierID << "|" << quantity << "|"
           << fixed << setprecision(2) << costEach << "|" << date;
        return ss.str();
    }
};

// ============================================================
//  ABSTRACT BASE CLASS: Alert
//  DERIVED:             ExpiryAlert, LowStockAlert
// ============================================================
class Alert {
protected:
    string m_stockID, m_medicineID, m_medicineName, m_message;

public:
    Alert(const string& sid, const string& mid, const string& mname, const string& msg)
        : m_stockID(sid), m_medicineID(mid), m_medicineName(mname), m_message(msg) {}

    virtual ~Alert() {}

    virtual string getAlertType() const = 0;

    virtual void display() const {
        cout << "  Stock ID  : " << m_stockID << "\n";
        cout << "  Medicine  : " << m_medicineName << " (" << m_medicineID << ")\n";
        cout << "  Message   : " << m_message << "\n";
    }

    virtual string serialize() const {
        return getAlertType() + "|" + m_stockID + "|" + m_medicineID + "|" + m_medicineName + "|" + m_message;
    }
};

class ExpiryAlert : public Alert {
private:
    string m_expiryDate;
    bool   m_alreadyExpired;

public:
    ExpiryAlert(const string& sid, const string& mid, const string& mname,
                const string& expiry, bool expired)
        : Alert(sid, mid, mname,
                expired ? "EXPIRED - Remove from physical shelf immediately!"
                        : "Expiring within 3 months - prioritize clearance/return."),
          m_expiryDate(expiry), m_alreadyExpired(expired) {}

    string getAlertType() const override {
        return m_alreadyExpired ? "EXPIRED" : "EXPIRING_SOON";
    }

    void display() const override {
        string col = m_alreadyExpired ? Color::RED : Color::YELLOW;
        cout << col << "\n  [ALERT: " << getAlertType() << "]\n" << Color::RESET;
        Alert::display();
        cout << "  Expiry    : " << m_expiryDate << "\n";
    }
};

class LowStockAlert : public Alert {
private:
    int m_currentQty, m_minStock;

public:
    LowStockAlert(const string& sid, const string& mid, const string& mname, int qty, int minStock)
        : Alert(sid, mid, mname, "Current shelf stock is below reorder threshold."),
          m_currentQty(qty), m_minStock(minStock) {}

    string getAlertType() const override { return "LOW_STOCK"; }

    void display() const override {
        cout << Color::YELLOW << "\n  [ALERT: LOW STOCK]\n" << Color::RESET;
        Alert::display();
        cout << "  Current   : " << m_currentQty << " units\n";
        cout << "  Minimum   : " << m_minStock   << " units\n";
    }
};

// ============================================================
//  CENTRAL INVENTORY SYSTEM CLASS
//  Coordinates Medicines, Physical Stock (Inventory), Suppliers,
//  and Transactions across 3 primary files.
// ============================================================
class Inventory {
private:
    vector<Medicine*>  m_medicines;   // Master Catalog
    vector<StockItem*> m_stocks;      // Physical Stock Items
    vector<Supplier*>  m_suppliers;   // Supplier Registry
    vector<Sale>       m_sales;
    vector<Purchase>   m_purchases;

    // Database Files
    const string F_MEDICINES = "pharma_medicines.txt";
    const string F_INVENTORY = "pharma_inventory.txt";
    const string F_SUPPLIERS = "pharma_suppliers.txt";
    const string F_SALES     = "pharma_sales.txt";
    const string F_PURCHASES = "pharma_purchases.txt";
    const string F_ALERTS    = "pharma_alerts.txt";

    int m_nextSaleID;
    int m_nextPurchaseID;
    int m_nextStockNum;
    bool m_silent;

    // Deserialise medicine line
    Medicine* parseMedicineLine(const string& line) {
        if (line.empty() || line[0] == '#') return nullptr;
        vector<string> p = split(line, '|');
        if (p.size() < 5) return nullptr;
        try {
            string type  = p[0];
            string id    = p[1];
            string name  = p[2];
            string mfr   = p[3];
            double price = stod(p[4]);

            if (type == "Tablet" && p.size() >= 7)
                return new Tablet(id, name, mfr, price, stoi(p[5]), stoi(p[6]));
            if (type == "Syrup" && p.size() >= 7)
                return new Syrup(id, name, mfr, price, stod(p[5]), p[6]);
            if (type == "Injection" && p.size() >= 7)
                return new Injection(id, name, mfr, price, stod(p[5]), p[6]);
        } catch (...) {}
        return nullptr;
    }

    void loadMedicines() {
        ifstream f(F_MEDICINES);
        if (!f) return;
        string line;
        while (getline(f, line)) {
            Medicine* m = parseMedicineLine(line);
            if (m) m_medicines.push_back(m);
        }
        if (!m_silent) printOK("Loaded " + to_string(m_medicines.size()) + " medicine catalog record(s).");
    }

    void loadInventory() {
        ifstream f(F_INVENTORY);
        if (!f) return;
        string line;
        while (getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            StockItem* st = StockItem::deserialize(line);
            if (st) {
                m_stocks.push_back(st);
                try {
                    int num = stoi(st->getStockID().substr(3));
                    if (num >= m_nextStockNum) m_nextStockNum = num + 1;
                } catch (...) {}
            }
        }
        if (!m_silent) printOK("Loaded " + to_string(m_stocks.size()) + " inventory stock record(s).");
    }

    void loadSuppliers() {
        ifstream f(F_SUPPLIERS);
        if (!f) return;
        string line;
        while (getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            Supplier* s = Supplier::deserialize(line);
            if (s) m_suppliers.push_back(s);
        }
        if (!m_silent) printOK("Loaded " + to_string(m_suppliers.size()) + " supplier(s).");
    }

    void loadSales() {
        ifstream f(F_SALES);
        if (!f) return;
        string line;
        while (getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            vector<string> p = split(line, '|');
            if (p.size() < 8) continue;
            try {
                Sale s;
                s.saleID       = p[0];
                s.stockID      = p[1];
                s.medicineID   = p[2];
                s.medicineName = p[3];
                s.customerName = p[4];
                s.quantity     = stoi(p[5]);
                s.priceEach    = stod(p[6]);
                s.date         = p[7];
                m_sales.push_back(s);

                int num = stoi(s.saleID.substr(1));
                if (num >= m_nextSaleID) m_nextSaleID = num + 1;
            } catch (...) {}
        }
    }

    void loadPurchases() {
        ifstream f(F_PURCHASES);
        if (!f) return;
        string line;
        while (getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            vector<string> p = split(line, '|');
            if (p.size() < 8) continue;
            try {
                Purchase pu;
                pu.purchaseID   = p[0];
                pu.stockID      = p[1];
                pu.medicineID   = p[2];
                pu.medicineName = p[3];
                pu.supplierID   = p[4];
                pu.quantity     = stoi(p[5]);
                pu.costEach     = stod(p[6]);
                pu.date         = p[7];
                m_purchases.push_back(pu);

                int num = stoi(pu.purchaseID.substr(1));
                if (num >= m_nextPurchaseID) m_nextPurchaseID = num + 1;
            } catch (...) {}
        }
    }

    void saveMedicines() const {
        ofstream f(F_MEDICINES);
        f << "# Pharma Inventory -- Medicines Catalog File\n";
        f << "# Format: Type|MedicineID|Name|Manufacturer|Price|[type-fields]\n";
        for (const Medicine* m : m_medicines)
            f << m->serialize() << "\n";
    }

    void saveInventory() const {
        ofstream f(F_INVENTORY);
        f << "# Pharma Inventory -- Physical Stock Ledger File\n";
        f << "# Format: StockID|MedicineID|SupplierID|BatchNumber|Quantity|MfgDate|Expiry(MM/YYYY)|MinStock\n";
        for (const StockItem* s : m_stocks)
            f << s->serialize() << "\n";
    }

    void saveSuppliers() const {
        ofstream f(F_SUPPLIERS);
        f << "# Pharma Inventory -- Suppliers File\n";
        f << "# Format: SupplierID|Name|Phone|Address|Company|Email\n";
        for (const Supplier* s : m_suppliers)
            f << s->serialize() << "\n";
    }

    void saveSales() const {
        ofstream f(F_SALES);
        f << "# Pharma Inventory -- Sales File\n";
        for (const Sale& s : m_sales)
            f << s.serialize() << "\n";
    }

    void savePurchases() const {
        ofstream f(F_PURCHASES);
        f << "# Pharma Inventory -- Purchases File\n";
        for (const Purchase& p : m_purchases)
            f << p.serialize() << "\n";
    }

public:
    Inventory(bool silent = false) : m_nextSaleID(1), m_nextPurchaseID(1), m_nextStockNum(1), m_silent(silent) {
        if (!m_silent) printSection("Loading 3-File Pharmacy Database");
        loadMedicines();
        loadInventory();
        loadSuppliers();
        loadSales();
        loadPurchases();
    }

    ~Inventory() {
        for (Medicine* m : m_medicines) delete m;
        for (StockItem* s : m_stocks)   delete s;
        for (Supplier* sup : m_suppliers) delete sup;
    }

    int getMedicineCount()  const { return (int)m_medicines.size();  }
    int getStockCount()     const { return (int)m_stocks.size();     }
    int getSupplierCount()  const { return (int)m_suppliers.size();  }
    int getSaleCount()      const { return (int)m_sales.size();      }
    int getPurchaseCount()  const { return (int)m_purchases.size();  }

    Medicine* findMedicine(const string& id) const {
        for (Medicine* m : m_medicines)
            if (m->getID() == id) return m;
        return nullptr;
    }

    Supplier* findSupplier(const string& id) const {
        for (Supplier* s : m_suppliers)
            if (s->getID() == id) return s;
        return nullptr;
    }

    StockItem* findStockItem(const string& stockID) const {
        for (StockItem* s : m_stocks)
            if (s->getStockID() == stockID) return s;
        return nullptr;
    }

    vector<StockItem*> findStocksForMedicine(const string& medID) const {
        vector<StockItem*> res;
        for (StockItem* s : m_stocks)
            if (s->getMedicineID() == medID) res.push_back(s);
        return res;
    }

    // ────────────────────────────────────────────────────────
    //  1. Display All Physical Inventory (Stock items on shelf)
    // ────────────────────────────────────────────────────────
    void displayAllInventory() const {
        printBanner("CURRENT PHYSICAL INVENTORY (STOCK LEDGER)");
        if (m_stocks.empty()) { printInfo("Inventory is empty."); return; }

        cout << "\n" << Color::BOLD
             << left << setw(9)  << "StockID"
             << setw(8)  << "MedID"
             << setw(20) << "Medicine Name"
             << setw(10) << "Batch"
             << setw(6)  << "Qty"
             << setw(11) << "Price"
             << setw(10) << "Expiry"
             << "Status"
             << Color::RESET << "\n"
             << string(82, '-') << "\n";

        for (const StockItem* s : m_stocks) {
            Medicine* m = findMedicine(s->getMedicineID());
            string name = m ? m->getName() : "Unknown";
            double pr   = m ? m->getPrice() : 0.0;

            string status, col;
            if (s->isExpired())          { status = "EXPIRED";    col = Color::RED;    }
            else if (s->isExpiringSoon()){ status = "EXP. SOON";  col = Color::YELLOW; }
            else if (s->isLowStock())    { status = "LOW STOCK";  col = Color::YELLOW; }
            else                         { status = "OK";         col = Color::GREEN;  }

            cout << left << setw(9)  << s->getStockID()
                 << setw(8)  << s->getMedicineID()
                 << setw(20) << (name.size() > 18 ? name.substr(0, 18) + ".." : name)
                 << setw(10) << s->getBatch()
                 << setw(6)  << s->getQuantity()
                 << "Rs." << setw(8) << fixed << setprecision(2) << pr
                 << setw(10) << s->getExpiry().toString()
                 << col << status << Color::RESET << "\n";
        }
    }

    // ────────────────────────────────────────────────────────
    //  2. View Medicine Details (Catalog Specs + Associated Batches)
    // ────────────────────────────────────────────────────────
    void viewMedicineDetail() const {
        printSection("View Medicine & Stock Details");
        cout << "  Enter Medicine ID: "; string id; getline(cin, id);
        Medicine* m = findMedicine(id);
        if (!m) { printErr("Medicine ID not found: " + id); return; }

        cout << "\n" << string(55, '-') << "\n";
        cout << Color::BOLD << "  [PRODUCT CATALOG SPECIFICATIONS]\n" << Color::RESET;
        m->display();   // Polymorphic virtual dispatch

        cout << "\n" << Color::BOLD << "  [PHYSICAL BATCHES IN INVENTORY]\n" << Color::RESET;
        vector<StockItem*> batches = findStocksForMedicine(id);
        if (batches.empty()) {
            printWarn("No physical stock currently available for this medicine.");
        } else {
            for (const StockItem* b : batches) {
                Supplier* sup = findSupplier(b->getSupplierID());
                cout << "\n";
                b->display(m, sup);
            }
        }
        cout << string(55, '-') << "\n";
    }

    // ────────────────────────────────────────────────────────
    //  3. Search Medicine
    // ────────────────────────────────────────────────────────
    void searchMedicine() const {
        printSection("Search Medicine Catalog");
        cout << "  Enter Medicine Name or ID: "; string query; getline(cin, query);
        string ql = query;
        transform(ql.begin(), ql.end(), ql.begin(), ::tolower);

        bool found = false;
        for (const Medicine* m : m_medicines) {
            string n = m->getName(), i = m->getID();
            transform(n.begin(), n.end(), n.begin(), ::tolower);
            transform(i.begin(), i.end(), i.begin(), ::tolower);

            if (n.find(ql) != string::npos || i.find(ql) != string::npos) {
                found = true;
                cout << "\n" << string(55, '=') << "\n";
                m->display();
                vector<StockItem*> batches = findStocksForMedicine(m->getID());
                cout << "  Available Batches in Inventory: " << batches.size() << "\n";
                int totalStock = 0;
                for (auto b : batches) totalStock += b->getQuantity();
                cout << "  Total Units in Stock          : " << totalStock << "\n";
            }
        }
        if (!found) printErr("No medicine matches: " + query);
    }

    // ────────────────────────────────────────────────────────
    //  4. Add New Medicine (Catalog Master + Optional Initial Stock)
    // ────────────────────────────────────────────────────────
    void addMedicine() {
        printSection("Add New Medicine to Catalog");
        cout << "  Type -- (1) Tablet  (2) Syrup  (3) Injection: ";
        int type; cin >> type; cin.ignore();
        if (type < 1 || type > 3) { printErr("Invalid medicine type."); return; }

        string id, name, mfr;
        double price;

        cout << "  Medicine ID        : "; getline(cin, id);
        if (findMedicine(id)) { printErr("Medicine ID already exists: " + id); return; }
        cout << "  Medicine Name      : "; getline(cin, name);
        cout << "  Manufacturer       : "; getline(cin, mfr);
        cout << "  Base Price (Rs.)   : "; cin >> price; cin.ignore();

        Medicine* med = nullptr;
        if (type == 1) {
            int mg, strips;
            cout << "  Strength (mg)      : "; cin >> mg;     cin.ignore();
            cout << "  Tablets per strip  : "; cin >> strips;  cin.ignore();
            med = new Tablet(id, name, mfr, price, mg, strips);
        } else if (type == 2) {
            double ml; string flavour;
            cout << "  Volume (ml/bottle) : "; cin >> ml;     cin.ignore();
            cout << "  Flavour            : "; getline(cin, flavour);
            med = new Syrup(id, name, mfr, price, ml, flavour);
        } else {
            double ml; string route;
            cout << "  Vial Volume (ml)   : "; cin >> ml;     cin.ignore();
            cout << "  Route (IV/IM/SC)   : "; getline(cin, route);
            med = new Injection(id, name, mfr, price, ml, route);
        }

        m_medicines.push_back(med);
        saveMedicines();
        printOK("Medicine saved to catalog (pharma_medicines.txt)!");

        // Prompt to add initial physical stock in pharma_inventory.txt
        cout << "\n  Add initial physical stock for this medicine now? (y/n): ";
        char ch; cin >> ch; cin.ignore();
        if (ch == 'y' || ch == 'Y') {
            cout << "  Supplier ID        : "; string supID; getline(cin, supID);
            cout << "  Batch Number       : "; string batch; getline(cin, batch);
            cout << "  Quantity           : "; int qty; cin >> qty; cin.ignore();
            cout << "  Mfg Date (DD/MM/YY): "; string mfg; getline(cin, mfg);
            cout << "  Expiry (MM/YYYY)   : "; string exp; getline(cin, exp);
            cout << "  Min Reorder Level  : "; int minStock; cin >> minStock; cin.ignore();

            ostringstream oss;
            oss << "STK" << setw(3) << setfill('0') << m_nextStockNum++;
            StockItem* st = new StockItem(oss.str(), id, supID, batch, qty, mfg, ExpiryDate::parse(exp), minStock);
            m_stocks.push_back(st);
            saveInventory();
            printOK("Physical stock registered in pharma_inventory.txt [" + oss.str() + "]");
        }
    }

    // ────────────────────────────────────────────────────────
    //  5. Update Medicine Price
    // ────────────────────────────────────────────────────────
    void updatePrice() {
        printSection("Update Medicine Catalog Price");
        cout << "  Enter Medicine ID: "; string id; getline(cin, id);
        Medicine* m = findMedicine(id);
        if (!m) { printErr("Medicine not found: " + id); return; }

        cout << "  Current Price: Rs. " << fixed << setprecision(2) << m->getPrice() << "\n";
        cout << "  New Price    : Rs. "; double p; cin >> p; cin.ignore();
        if (p < 0) { printErr("Price cannot be negative."); return; }

        m->setPrice(p);
        saveMedicines();
        printOK("Price updated in pharma_medicines.txt!");
    }

    // ────────────────────────────────────────────────────────
    //  6. Sell Medicine (Stock deduction from physical inventory)
    // ────────────────────────────────────────────────────────
    void sellMedicine() {
        printSection("Dispense / Sell Medicine");
        cout << "  Enter Stock ID or Medicine ID: "; string id; getline(cin, id);

        StockItem* stock = findStockItem(id);
        if (!stock) {
            // Check if user entered Medicine ID
            vector<StockItem*> batches = findStocksForMedicine(id);
            if (batches.empty()) {
                printErr("No physical stock found for: " + id);
                return;
            }
            if (batches.size() == 1) {
                stock = batches[0];
            } else {
                cout << "\n  Multiple batches found for this medicine in inventory:\n";
                for (size_t i = 0; i < batches.size(); ++i) {
                    cout << "  [" << (i + 1) << "] Stock: " << batches[i]->getStockID()
                         << " | Batch: " << batches[i]->getBatch()
                         << " | Qty: " << batches[i]->getQuantity()
                         << " | Expiry: " << batches[i]->getExpiry().toString() << "\n";
                }
                cout << "  Select batch (1-" << batches.size() << "): ";
                size_t sel; cin >> sel; cin.ignore();
                if (sel < 1 || sel > batches.size()) { printErr("Invalid selection."); return; }
                stock = batches[sel - 1];
            }
        }

        // SAFETY GATE: BLOCK SALE IF EXPIRED
        if (stock->isExpired()) {
            printErr("PATIENT SAFETY BLOCK: Batch " + stock->getBatch() + " has EXPIRED! Sale rejected.");
            return;
        }

        if (stock->isExpiringSoon())
            printWarn("Alert: This batch is expiring within 3 months.");

        Medicine* med = findMedicine(stock->getMedicineID());
        double price = med ? med->getPrice() : 0.0;
        string medName = med ? med->getName() : "Medicine";

        cout << "\n";
        stock->display(med, findSupplier(stock->getSupplierID()));

        cout << "\n  Customer Name    : "; string customer; getline(cin, customer);
        cout << "  Quantity to sell : "; int qty; cin >> qty; cin.ignore();

        if (qty <= 0) { printErr("Quantity must be positive."); return; }
        if (qty > stock->getQuantity()) {
            printErr("Insufficient stock! Available: " + to_string(stock->getQuantity()));
            return;
        }

        // Deduct physical inventory
        stock->setQuantity(stock->getQuantity() - qty);

        // Record Sale
        Sale s;
        s.saleID       = "S" + to_string(m_nextSaleID++);
        s.stockID      = stock->getStockID();
        s.medicineID   = stock->getMedicineID();
        s.medicineName = medName;
        s.customerName = customer;
        s.quantity     = qty;
        s.priceEach    = price;
        s.date         = currentDate();
        m_sales.push_back(s);

        saveInventory();
        saveSales();

        printOK("Transaction completed successfully!");
        cout << "\n"; s.display(); cout << "\n";

        if (stock->isLowStock())
            printWarn("WARNING: Batch " + stock->getBatch() + " is now below safe reorder level!");
    }

    // ────────────────────────────────────────────────────────
    //  7. Receive Stock (Restock physical inventory)
    // ────────────────────────────────────────────────────────
    void receiveStock() {
        printSection("Receive Stock / Procurement");
        cout << "  Enter Medicine ID: "; string medID; getline(cin, medID);
        Medicine* med = findMedicine(medID);
        if (!med) { printErr("Medicine ID not registered in catalog."); return; }

        cout << "  Restock option:\n";
        cout << "    [1] Restock an existing Stock/Batch entry\n";
        cout << "    [2] Register a new Batch in physical inventory\n";
        cout << "  Enter choice: "; int opt; cin >> opt; cin.ignore();

        if (opt == 1) {
            vector<StockItem*> batches = findStocksForMedicine(medID);
            if (batches.empty()) { printErr("No existing batches found. Please add a new batch."); return; }

            for (size_t i = 0; i < batches.size(); ++i) {
                cout << "  [" << (i + 1) << "] " << batches[i]->getStockID()
                     << " (Batch: " << batches[i]->getBatch() << ", Qty: "
                     << batches[i]->getQuantity() << ")\n";
            }
            cout << "  Select batch (1-" << batches.size() << "): ";
            size_t sel; cin >> sel; cin.ignore();
            if (sel < 1 || sel > batches.size()) { printErr("Invalid choice."); return; }

            StockItem* st = batches[sel - 1];
            cout << "  Quantity received : "; int qty; cin >> qty; cin.ignore();
            cout << "  Cost per unit     : Rs. "; double cost; cin >> cost; cin.ignore();

            st->setQuantity(st->getQuantity() + qty);

            Purchase pu;
            pu.purchaseID   = "P" + to_string(m_nextPurchaseID++);
            pu.stockID      = st->getStockID();
            pu.medicineID   = medID;
            pu.medicineName = med->getName();
            pu.supplierID   = st->getSupplierID();
            pu.quantity     = qty;
            pu.costEach     = cost;
            pu.date         = currentDate();
            m_purchases.push_back(pu);

            saveInventory();
            savePurchases();
            printOK("Stock updated in pharma_inventory.txt!");
            cout << "\n"; pu.display();

        } else if (opt == 2) {
            cout << "  Supplier ID        : "; string supID; getline(cin, supID);
            cout << "  Batch Number       : "; string batch; getline(cin, batch);
            cout << "  Quantity received  : "; int qty; cin >> qty; cin.ignore();
            cout << "  Cost per unit      : Rs. "; double cost; cin >> cost; cin.ignore();
            cout << "  Mfg Date (DD/MM/YY): "; string mfg; getline(cin, mfg);
            cout << "  Expiry (MM/YYYY)   : "; string exp; getline(cin, exp);
            cout << "  Min Reorder Level  : "; int minStock; cin >> minStock; cin.ignore();

            ostringstream oss;
            oss << "STK" << setw(3) << setfill('0') << m_nextStockNum++;
            string stockID = oss.str();

            StockItem* st = new StockItem(stockID, medID, supID, batch, qty, mfg, ExpiryDate::parse(exp), minStock);
            m_stocks.push_back(st);

            Purchase pu;
            pu.purchaseID   = "P" + to_string(m_nextPurchaseID++);
            pu.stockID      = stockID;
            pu.medicineID   = medID;
            pu.medicineName = med->getName();
            pu.supplierID   = supID;
            pu.quantity     = qty;
            pu.costEach     = cost;
            pu.date         = currentDate();
            m_purchases.push_back(pu);

            saveInventory();
            savePurchases();
            printOK("New batch added to pharma_inventory.txt [" + stockID + "]!");
            cout << "\n"; pu.display();
        } else {
            printErr("Invalid option.");
        }
    }

    // ────────────────────────────────────────────────────────
    //  8. Generate System Alerts (Polymorphic Alert objects)
    // ────────────────────────────────────────────────────────
    void generateAlerts() {
        printSection("System Alerts & Auditing");
        vector<Alert*> alerts;   // Polymorphism: vector holding base-class pointers

        for (const StockItem* s : m_stocks) {
            Medicine* m = findMedicine(s->getMedicineID());
            string mname = m ? m->getName() : "Medicine";

            if (s->isExpired())
                alerts.push_back(new ExpiryAlert(s->getStockID(), s->getMedicineID(), mname, s->getExpiry().toString(), true));
            else if (s->isExpiringSoon())
                alerts.push_back(new ExpiryAlert(s->getStockID(), s->getMedicineID(), mname, s->getExpiry().toString(), false));

            if (s->isLowStock())
                alerts.push_back(new LowStockAlert(s->getStockID(), s->getMedicineID(), mname, s->getQuantity(), s->getMinStock()));
        }

        ofstream af(F_ALERTS);
        af << "# Pharma Alerts -- Generated on: " << currentDate() << "\n";
        af << "# Format: AlertType|StockID|MedicineID|MedicineName|Message\n";

        if (alerts.empty()) {
            printOK("Zero safety alerts! Physical inventory is fully safe and stocked.");
        } else {
            printWarn(to_string(alerts.size()) + " alert(s) requiring attention:");
            for (Alert* a : alerts) {
                a->display();               // Dynamic polymorphic dispatch
                af << a->serialize() << "\n";
                delete a;                  // Memory cleanup
            }
            printInfo("Alert report exported to: " + F_ALERTS);
        }
        alerts.clear();
    }

    // ────────────────────────────────────────────────────────
    //  9. Expiry Report
    // ────────────────────────────────────────────────────────
    void expiryReport() const {
        printBanner("EXPIRY AUDIT REPORT");
        bool any = false;
        for (const StockItem* s : m_stocks) {
            if (!s->isExpired() && !s->isExpiringSoon()) continue;
            any = true;
            Medicine* m = findMedicine(s->getMedicineID());
            string name = m ? m->getName() : "Unknown";

            string tag = s->isExpired() ? "EXPIRED" : "EXPIRING SOON";
            string col = s->isExpired() ? Color::RED : Color::YELLOW;

            cout << col << "\n  [" << tag << "]" << Color::RESET << "\n";
            cout << "  Stock ID : " << s->getStockID() << "\n";
            cout << "  Medicine : " << name << " (" << s->getMedicineID() << ")\n";
            cout << "  Batch    : " << s->getBatch() << "\n";
            cout << "  Quantity : " << s->getQuantity() << " units\n";
            cout << "  Expiry   : " << s->getExpiry().toString() << "\n";
            cout << string(45, '-') << "\n";
        }
        if (!any) printOK("No stock batches are expired or expiring soon.");
    }

    // ────────────────────────────────────────────────────────
    //  10. Sales Report
    // ────────────────────────────────────────────────────────
    void salesReport() const {
        printBanner("SALES REVENUE REPORT");
        if (m_sales.empty()) { printInfo("No sales recorded yet."); return; }
        double total = 0;
        for (const Sale& s : m_sales) {
            cout << "\n"; s.display();
            cout << string(45, '-') << "\n";
            total += (s.quantity * s.priceEach);
        }
        cout << Color::GREEN << "\n  TOTAL REVENUE GENERATED: Rs. "
             << fixed << setprecision(2) << total << Color::RESET << "\n";
    }

    // ────────────────────────────────────────────────────────
    //  11. Purchase Report
    // ────────────────────────────────────────────────────────
    void purchaseReport() const {
        printBanner("PROCUREMENT EXPENDITURE REPORT");
        if (m_purchases.empty()) { printInfo("No purchases recorded yet."); return; }
        double total = 0;
        for (const Purchase& p : m_purchases) {
            cout << "\n"; p.display();
            cout << string(45, '-') << "\n";
            total += (p.quantity * p.costEach);
        }
        cout << Color::RED << "\n  TOTAL PROCUREMENT COST: Rs. "
             << fixed << setprecision(2) << total << Color::RESET << "\n";
    }

    // ────────────────────────────────────────────────────────
    //  12. Supplier Management
    // ────────────────────────────────────────────────────────
    void addSupplier() {
        printSection("Register New Supplier");
        string id, name, phone, address, company, email;
        cout << "  Supplier ID : "; getline(cin, id);
        if (findSupplier(id)) { printErr("Supplier ID already exists: " + id); return; }
        cout << "  Name        : "; getline(cin, name);
        cout << "  Phone       : "; getline(cin, phone);
        cout << "  Address     : "; getline(cin, address);
        cout << "  Company     : "; getline(cin, company);
        cout << "  Email       : "; getline(cin, email);

        m_suppliers.push_back(new Supplier(id, name, phone, address, company, email));
        saveSuppliers();
        printOK("Supplier saved to pharma_suppliers.txt: " + name);
    }

    void displaySuppliers() const {
        printSection("Supplier Directory");
        if (m_suppliers.empty()) { printInfo("No suppliers registered."); return; }
        for (const Person* p : m_suppliers) {   // Base-class pointer
            p->display();                        // Polymorphic dispatch
            cout << string(45, '-') << "\n";
        }
    }

    // ────────────────────────────────────────────────────────
    //  13. View Medicine Catalog Only (Definitions)
    // ────────────────────────────────────────────────────────
    void displayMedicineCatalog() const {
        printBanner("MEDICINE PRODUCT CATALOG (DEFINITIONS)");
        if (m_medicines.empty()) { printInfo("Catalog is empty."); return; }

        cout << "\n" << Color::BOLD
             << left << setw(8)  << "ID"
             << setw(24) << "Name"
             << setw(12) << "Type"
             << setw(20) << "Manufacturer"
             << "Base Price"
             << Color::RESET << "\n"
             << string(74, '-') << "\n";

        for (const Medicine* m : m_medicines) {
            cout << left << setw(8)  << m->getID()
                 << setw(24) << m->getName()
                 << setw(12) << m->getType()
                 << setw(20) << m->getManufacturer()
                 << "Rs. " << fixed << setprecision(2) << m->getPrice() << "\n";
        }
    }

    // ────────────────────────────────────────────────────────
    //  CLI API Methods for Desktop Application Integration
    // ────────────────────────────────────────────────────────
    void apiGetStock() const {
        cout << "[\n";
        for (size_t i = 0; i < m_stocks.size(); ++i) {
            const StockItem* s = m_stocks[i];
            const Medicine* m = findMedicine(s->getMedicineID());
            const Supplier* sup = findSupplier(s->getSupplierID());
            cout << "  {"
                 << "\"stockID\":\"" << s->getStockID() << "\","
                 << "\"medicineID\":\"" << s->getMedicineID() << "\","
                 << "\"medicineName\":\"" << (m ? m->getName() : "Unknown") << "\","
                 << "\"type\":\"" << (m ? m->getType() : "N/A") << "\","
                 << "\"supplierID\":\"" << s->getSupplierID() << "\","
                 << "\"supplierName\":\"" << (sup ? sup->getCompany() : s->getSupplierID()) << "\","
                 << "\"batch\":\"" << s->getBatch() << "\","
                 << "\"quantity\":" << s->getQuantity() << ","
                 << "\"price\":" << (m ? m->getPrice() : 0.0) << ","
                 << "\"mfgDate\":\"" << s->getMfgDate() << "\","
                 << "\"expiry\":\"" << s->getExpiry().toString() << "\","
                 << "\"minStock\":" << s->getMinStock() << ","
                 << "\"isExpired\":" << (s->isExpired() ? "true" : "false") << ","
                 << "\"isExpiringSoon\":" << (s->isExpiringSoon() ? "true" : "false") << ","
                 << "\"isLowStock\":" << (s->isLowStock() ? "true" : "false")
                 << "}" << (i + 1 < m_stocks.size() ? "," : "") << "\n";
        }
        cout << "]\n";
    }

    void apiGetCatalog() const {
        cout << "[\n";
        for (size_t i = 0; i < m_medicines.size(); ++i) {
            const Medicine* m = m_medicines[i];
            cout << "  {"
                 << "\"id\":\"" << m->getID() << "\","
                 << "\"name\":\"" << m->getName() << "\","
                 << "\"type\":\"" << m->getType() << "\","
                 << "\"manufacturer\":\"" << m->getManufacturer() << "\","
                 << "\"price\":" << m->getPrice() << ","
                 << "\"usage\":\"" << m->getUsageInstructions() << "\""
                 << "}" << (i + 1 < m_medicines.size() ? "," : "") << "\n";
        }
        cout << "]\n";
    }

    void apiGetSuppliers() const {
        cout << "[\n";
        for (size_t i = 0; i < m_suppliers.size(); ++i) {
            const Supplier* s = m_suppliers[i];
            cout << "  {"
                 << "\"id\":\"" << s->getID() << "\","
                 << "\"company\":\"" << s->getCompany() << "\","
                 << "\"name\":\"" << s->getName() << "\","
                 << "\"phone\":\"" << s->getPhone() << "\","
                 << "\"email\":\"" << s->getEmail() << "\","
                 << "\"address\":\"" << s->getAddress() << "\""
                 << "}" << (i + 1 < m_suppliers.size() ? "," : "") << "\n";
        }
        cout << "]\n";
    }

    void apiSell(const string& stockID, int qty, const string& customer) {
        StockItem* stock = findStockItem(stockID);
        if (!stock) {
            cout << "{\"success\":false,\"error\":\"Stock batch not found: " << stockID << "\"}\n";
            return;
        }
        if (stock->isExpired()) {
            cout << "{\"success\":false,\"error\":\"PATIENT SAFETY VIOLATION: Batch " << stock->getBatch()
                 << " has EXPIRED! C++ safety gate has rejected the sale.\"}\n";
            return;
        }
        if (qty <= 0) {
            cout << "{\"success\":false,\"error\":\"Quantity must be greater than zero.\"}\n";
            return;
        }
        if (qty > stock->getQuantity()) {
            cout << "{\"success\":false,\"error\":\"Insufficient stock. Available: " << stock->getQuantity() << "\"}\n";
            return;
        }

        Medicine* med = findMedicine(stock->getMedicineID());
        double price = med ? med->getPrice() : 0.0;
        string mname = med ? med->getName() : "Unknown";

        stock->setQuantity(stock->getQuantity() - qty);

        Sale s;
        s.saleID = "S" + to_string(m_nextSaleID++);
        s.stockID = stock->getStockID();
        s.medicineID = stock->getMedicineID();
        s.medicineName = mname;
        s.customerName = customer;
        s.quantity = qty;
        s.priceEach = price;
        s.date = currentDate();
        m_sales.push_back(s);

        saveInventory();
        saveSales();

        cout << "{\"success\":true,\"saleID\":\"" << s.saleID << "\","
             << "\"stockID\":\"" << s.stockID << "\","
             << "\"medicineName\":\"" << s.medicineName << "\","
             << "\"customer\":\"" << s.customerName << "\","
             << "\"quantity\":" << qty << ","
             << "\"priceEach\":" << price << ","
             << "\"total\":" << (qty * price) << ","
             << "\"date\":\"" << s.date << "\"}\n";
    }

    void apiReceive(const string& medID, const string& supID, const string& batch,
                    int qty, double cost, const string& mfg, const string& exp, int minStock) {
        Medicine* med = findMedicine(medID);
        if (!med) {
            cout << "{\"success\":false,\"error\":\"Medicine not found in catalog.\"}\n";
            return;
        }
        ostringstream oss;
        oss << "STK" << setw(3) << setfill('0') << m_nextStockNum++;
        string stockID = oss.str();

        StockItem* st = new StockItem(stockID, medID, supID, batch, qty, mfg, ExpiryDate::parse(exp), minStock);
        m_stocks.push_back(st);

        Purchase pu;
        pu.purchaseID = "P" + to_string(m_nextPurchaseID++);
        pu.stockID = stockID;
        pu.medicineID = medID;
        pu.medicineName = med->getName();
        pu.supplierID = supID;
        pu.quantity = qty;
        pu.costEach = cost;
        pu.date = currentDate();
        m_purchases.push_back(pu);

        saveInventory();
        savePurchases();

        cout << "{\"success\":true,\"stockID\":\"" << stockID << "\","
             << "\"purchaseID\":\"" << pu.purchaseID << "\"}\n";
    }

    void apiAddMedicine(const string& type, const string& id, const string& name,
                        const string& mfr, double price, const string& f1, const string& f2) {
        if (findMedicine(id)) {
            cout << "{\"success\":false,\"error\":\"Medicine ID already exists: " << id << "\"}\n";
            return;
        }
        Medicine* med = nullptr;
        if (type == "Tablet") {
            med = new Tablet(id, name, mfr, price, stoi(f1), stoi(f2));
        } else if (type == "Syrup") {
            med = new Syrup(id, name, mfr, price, stod(f1), f2);
        } else if (type == "Injection") {
            med = new Injection(id, name, mfr, price, stod(f1), f2);
        } else {
            cout << "{\"success\":false,\"error\":\"Invalid type.\"}\n";
            return;
        }
        m_medicines.push_back(med);
        saveMedicines();
        cout << "{\"success\":true,\"id\":\"" << id << "\"}\n";
    }

    void apiAddSupplier(const string& id, const string& comp, const string& name,
                        const string& phone, const string& email, const string& addr) {
        if (findSupplier(id)) {
            cout << "{\"success\":false,\"error\":\"Supplier ID already exists: " << id << "\"}\n";
            return;
        }
        m_suppliers.push_back(new Supplier(id, name, phone, addr, comp, email));
        saveSuppliers();
        cout << "{\"success\":true,\"id\":\"" << id << "\"}\n";
    }

    void apiAlerts() {
        vector<Alert*> alerts;
        for (const StockItem* s : m_stocks) {
            Medicine* m = findMedicine(s->getMedicineID());
            string mname = m ? m->getName() : "Medicine";
            if (s->isExpired())
                alerts.push_back(new ExpiryAlert(s->getStockID(), s->getMedicineID(), mname, s->getExpiry().toString(), true));
            else if (s->isExpiringSoon())
                alerts.push_back(new ExpiryAlert(s->getStockID(), s->getMedicineID(), mname, s->getExpiry().toString(), false));
            if (s->isLowStock())
                alerts.push_back(new LowStockAlert(s->getStockID(), s->getMedicineID(), mname, s->getQuantity(), s->getMinStock()));
        }

        ofstream af(F_ALERTS);
        af << "# Pharma Alerts -- Generated on: " << currentDate() << "\n";
        cout << "[\n";
        for (size_t i = 0; i < alerts.size(); ++i) {
            af << alerts[i]->serialize() << "\n";
            cout << "  {"
                 << "\"type\":\"" << alerts[i]->getAlertType() << "\","
                 << "\"serialized\":\"" << alerts[i]->serialize() << "\""
                 << "}" << (i + 1 < alerts.size() ? "," : "") << "\n";
            delete alerts[i];
        }
        cout << "]\n";
    }

    void apiReports() const {
        double totalSales = 0, totalPurchases = 0;
        for (const Sale& s : m_sales) totalSales += (s.quantity * s.priceEach);
        for (const Purchase& p : m_purchases) totalPurchases += (p.quantity * p.costEach);

        cout << "{\n"
             << "  \"totalRevenue\":" << totalSales << ",\n"
             << "  \"totalExpenses\":" << totalPurchases << ",\n"
             << "  \"salesCount\":" << m_sales.size() << ",\n"
             << "  \"purchasesCount\":" << m_purchases.size() << ",\n"
             << "  \"sales\": [\n";
        for (size_t i = 0; i < m_sales.size(); ++i) {
            const Sale& s = m_sales[i];
            cout << "    {\"saleID\":\"" << s.saleID << "\",\"stockID\":\"" << s.stockID
                 << "\",\"medicineName\":\"" << s.medicineName << "\",\"customer\":\"" << s.customerName
                 << "\",\"quantity\":" << s.quantity << ",\"total\":" << (s.quantity * s.priceEach)
                 << ",\"date\":\"" << s.date << "\"}" << (i + 1 < m_sales.size() ? "," : "") << "\n";
        }
        cout << "  ],\n  \"purchases\": [\n";
        for (size_t i = 0; i < m_purchases.size(); ++i) {
            const Purchase& p = m_purchases[i];
            cout << "    {\"purchaseID\":\"" << p.purchaseID << "\",\"stockID\":\"" << p.stockID
                 << "\",\"medicineName\":\"" << p.medicineName << "\",\"supplierID\":\"" << p.supplierID
                 << "\",\"quantity\":" << p.quantity << ",\"total\":" << (p.quantity * p.costEach)
                 << ",\"date\":\"" << p.date << "\"}" << (i + 1 < m_purchases.size() ? "," : "") << "\n";
        }
        cout << "  ]\n}\n";
    }
};

// ============================================================
//  Seed Initial 3-File Sample Data (First run only)
// ============================================================
void seedDemoData() {
    // 1. Suppliers
    ofstream sf("pharma_suppliers.txt");
    sf << "# Pharma Inventory -- Suppliers File\n";
    sf << "# Format: SupplierID|Name|Phone|Address|Company|Email\n";
    sf << "SUP001|Rahul Kumar|9876543210|Mumbai, Maharashtra|MedCo Pharma|rahul@medco.com\n";
    sf << "SUP002|Priya Singh|8765432109|New Delhi|HealthPlus Ltd|priya@healthplus.in\n";
    sf.close();

    // 2. Medicine Catalog
    ofstream mf("pharma_medicines.txt");
    mf << "# Pharma Inventory -- Medicines Catalog File (Auto-Generated)\n";
    mf << "# Format: Type|MedicineID|Name|Manufacturer|Price|[type-fields]\n";
    mf << "Tablet|M001|Paracetamol 500mg|Cipla|25.00|500|10\n";
    mf << "Tablet|M002|Amoxicillin 250mg|Sun Pharma|80.00|250|14\n";
    mf << "Tablet|M003|Metformin 500mg|Dr. Reddys|45.00|500|14\n";
    mf << "Syrup|M004|Cetirizine Syrup|Abbott|40.00|60.0|Mint\n";
    mf << "Syrup|M005|Cough Syrup|Pfizer|55.00|100.0|Honey\n";
    mf << "Injection|M006|Insulin Regular|Novo Nordisk|350.00|10.0|IV\n";
    mf << "Injection|M007|Amikacin 500mg|Cipla|120.00|2.0|IM\n";
    mf.close();

    // 3. Physical Inventory Ledger
    ofstream invf("pharma_inventory.txt");
    invf << "# Pharma Inventory -- Physical Stock Ledger File (Auto-Generated)\n";
    invf << "# Format: StockID|MedicineID|SupplierID|BatchNumber|Quantity|MfgDate|Expiry(MM/YYYY)|MinStock\n";
    invf << "STK001|M001|SUP001|P101|100|01/01/2024|03/2027|20\n";
    invf << "STK002|M002|SUP001|A205|50|15/06/2023|11/2026|15\n";
    invf << "STK003|M003|SUP002|M310|30|01/03/2024|06/2027|10\n";
    invf << "STK004|M004|SUP002|C410|25|01/08/2024|08/2027|10\n";
    invf << "STK005|M005|SUP001|CS501|8|01/04/2023|02/2025|10\n";
    invf << "STK006|M006|SUP002|I601|20|01/09/2024|12/2026|5\n";
    invf << "STK007|M007|SUP001|AM701|3|01/07/2023|09/2026|5\n";
    invf.close();

    printOK("3-file normalized database initialized.");
}

// ============================================================
//  Menu Display
// ============================================================
void showMenu(const Inventory& inv) {
    printBanner("PHARMA INVENTORY MANAGEMENT SYSTEM (3-FILE ARCHITECTURE)");
    cout << Color::WHITE
         << "  Medicines: " << inv.getMedicineCount()
         << "  |  Stock Batches: " << inv.getStockCount()
         << "  |  Suppliers: " << inv.getSupplierCount()
         << "  |  Sales: " << inv.getSaleCount()
         << "  |  Purchases: " << inv.getPurchaseCount()
         << Color::RESET << "\n";

    cout << Color::BOLD << "\n  --- PHYSICAL INVENTORY (pharma_inventory.txt) ---\n" << Color::RESET;
    cout << "  [1]  View All Physical Inventory (Batches & Stock Levels)\n";
    cout << "  [2]  View Medicine Detail (Specs + Active Batches)\n";
    cout << "  [3]  Search Medicine\n";
    cout << "  [4]  Add New Medicine (Catalog & Initial Stock)\n";
    cout << "  [5]  Update Medicine Catalog Price\n";
    cout << "  [6]  View Medicine Catalog (pharma_medicines.txt)\n";

    cout << Color::BOLD << "\n  --- TRANSACTIONS & STOCK MOVEMENT ---\n" << Color::RESET;
    cout << "  [7]  Sell / Dispense Medicine (Safety Expiry Check)\n";
    cout << "  [8]  Receive Stock / Restock Batch (Purchase)\n";

    cout << Color::BOLD << "\n  --- SUPPLIERS (pharma_suppliers.txt) ---\n" << Color::RESET;
    cout << "  [9]  View All Suppliers\n";
    cout << "  [10] Register New Supplier\n";

    cout << Color::BOLD << "\n  --- REPORTS & AUDITS ---\n" << Color::RESET;
    cout << "  [11] Expiry Report\n";
    cout << "  [12] Sales Revenue Report\n";
    cout << "  [13] Procurement Cost Report\n";
    cout << "  [14] Generate System Safety Alerts (Export to file)\n";

    cout << Color::RED << "\n  [0]  Save & Exit\n" << Color::RESET;
    cout << "\n  " << Color::BOLD << "Enter your choice: " << Color::RESET;
}

// ============================================================
//  main() — Supports both Interactive Console & Desktop CLI API
// ============================================================
int main(int argc, char* argv[]) {
    // Ensure database files exist
    {
        ifstream f1("pharma_medicines.txt");
        ifstream f2("pharma_inventory.txt");
        ifstream f3("pharma_suppliers.txt");
        if (!f1.good() || !f2.good() || !f3.good()) {
            seedDemoData();
        }
    }

    // CLI API mode for Desktop Application integration
    if (argc > 1) {
        Inventory inv(true);
        string cmd = argv[1];
        if (cmd == "--api-stock") {
            inv.apiGetStock();
            return 0;
        } else if (cmd == "--api-catalog") {
            inv.apiGetCatalog();
            return 0;
        } else if (cmd == "--api-suppliers") {
            inv.apiGetSuppliers();
            return 0;
        } else if (cmd == "--api-sell" && argc >= 5) {
            inv.apiSell(argv[2], stoi(argv[3]), argv[4]);
            return 0;
        } else if (cmd == "--api-receive" && argc >= 10) {
            inv.apiReceive(argv[2], argv[3], argv[4], stoi(argv[5]), stod(argv[6]), argv[7], argv[8], stoi(argv[9]));
            return 0;
        } else if (cmd == "--api-add-med" && argc >= 9) {
            inv.apiAddMedicine(argv[2], argv[3], argv[4], argv[5], stod(argv[6]), argv[7], argv[8]);
            return 0;
        } else if (cmd == "--api-add-sup" && argc >= 8) {
            inv.apiAddSupplier(argv[2], argv[3], argv[4], argv[5], argv[6], argv[7]);
            return 0;
        } else if (cmd == "--api-alerts") {
            inv.apiAlerts();
            return 0;
        } else if (cmd == "--api-reports") {
            inv.apiReports();
            return 0;
        }
    }

    printBanner("PHARMA INVENTORY SYSTEM INITIALIZATION");
    Inventory inv;

    int choice = -1;
    do {
        showMenu(inv);

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(1000, '\n');
            printErr("Invalid input. Please enter a number.");
            continue;
        }
        cin.ignore();

        switch (choice) {
            case 1:  inv.displayAllInventory();     break;
            case 2:  inv.viewMedicineDetail();      break;
            case 3:  inv.searchMedicine();          break;
            case 4:  inv.addMedicine();             break;
            case 5:  inv.updatePrice();             break;
            case 6:  inv.displayMedicineCatalog();  break;
            case 7:  inv.sellMedicine();            break;
            case 8:  inv.receiveStock();            break;
            case 9:  inv.displaySuppliers();        break;
            case 10: inv.addSupplier();             break;
            case 11: inv.expiryReport();            break;
            case 12: inv.salesReport();             break;
            case 13: inv.purchaseReport();          break;
            case 14: inv.generateAlerts();          break;
            case 0:
                printBanner("System Data Saved Successfully");
                printOK("pharma_medicines.txt  [Catalog saved]");
                printOK("pharma_inventory.txt  [Physical stock saved]");
                printOK("pharma_suppliers.txt  [Suppliers saved]");
                printOK("pharma_sales.txt      [Sales ledger saved]");
                printOK("pharma_purchases.txt  [Purchases ledger saved]");
                printInfo("Exiting Pharma Inventory Management System.");
                break;
            default:
                printErr("Invalid choice. Please enter a valid number (0-14).");
        }

        if (choice != 0) {
            cout << "\n" << Color::BOLD << "  Press ENTER to continue..." << Color::RESET;
            cin.get();
        }

    } while (choice != 0);

    return 0;
}
