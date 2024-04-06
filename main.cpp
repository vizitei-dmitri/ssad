#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <fstream>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <unordered_map>
#include <sstream>
using namespace std;
#include <limits>
class Character;
class Weapon;
class Potion;
class Spell;

class Narrator {
    std::ofstream logFile;

public:
    Narrator(const std::string& filename) : logFile(filename) {
        if (!logFile.is_open()) {
            throw std::runtime_error("Failed to open log file.");
        }
    }

    void logEvent(const std::string& event) {
        logFile << event << std::endl;
    }

    ~Narrator() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
};

// Global Narrator instance
Narrator narrator("story_log.txt");


class PhysicalItem {
protected:
    shared_ptr<Character> owner;
    string name;

public:
    PhysicalItem(string name, Character* owner, bool isUsableOnce)
        : name(name), owner(owner), isUsableOnce(isUsableOnce) {}

    virtual ~PhysicalItem() {}
    bool isUsableOnce;
    virtual void use(Character* user, Character* target) = 0;
    string getName() const { return name; }
    virtual void setup() = 0;
protected:
    virtual void print(ostream& os) const = 0;
};



// Abstract Character class
class Character {
    friend class PhysicalItem;
protected:
    int healthPoints;
    string name;
public:
    Character(string name, int hp) : name(name), healthPoints(hp) {}
    virtual ~Character() {}

    bool isAlive() const {
        return healthPoints > 0;
    }
    virtual std::string getType() const = 0;
    virtual bool addItem(std::unique_ptr<PhysicalItem> item) = 0;
    virtual bool canCarryWeapon() const { return true; }
    virtual bool canCarryPotion() const { return true; }
    virtual bool canCarrySpell() const { return true; }

    string getName() const { return name; }
    int getHP() const { return healthPoints; }

    void heal(int healValue) {
        healthPoints += healValue;
    }

    virtual void print(ostream& os) const {
        os << "Character: " << name << " HP: " << healthPoints << endl;
    }
    void takeDamage(int damage) {
        if (healthPoints <= 0) {
            return;
        }
        healthPoints -= damage;
        if (healthPoints <= 0) {
            healthPoints = 0;

            narrator.logEvent(name + " has died.");
        }
    }
};
template<typename T>
concept DerivedFromPhysicalItem = is_base_of<PhysicalItem, T>::value;

template<typename T>
class Container{
private:
    map<string, unique_ptr<T>> elements;
    int maxCapacity;
public:

};


template<DerivedFromPhysicalItem T>
class Container<T>{
protected:
    vector<T> elems;
public:
    Container(){
        cout << "Base container created"<<endl;
    }
    ~Container(){
        cout << "Base container destroyedwha" << endl;
    }
private:
    map<string, unique_ptr<T>> elements;
    int maxCapacity;
public:
    Container(int size) : maxCapacity(size) {}

    bool addItem(unique_ptr<T> newItem) {
        if (elements.size() >= maxCapacity) {
            narrator.logEvent("Error caught: Container is full. Cannot add " + newItem->getName() + ".");
            return false;
        }
        elements.insert({newItem->getName(), std::move(newItem)});
        return true;
    }


    T* getItem(const string& itemName) {
        auto it = elements.find(itemName);
        if (it != elements.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    bool removeItem(const string& itemName) {
        return elements.erase(itemName) > 0;
    }

    void print() const {
        for (const auto& pair : elements) {
            pair.second->print(cout);
        }
    }
};




class Spell : public PhysicalItem {
    vector<Character*> allowedTargets;
public:
    Spell(string name, Character* owner, vector<Character*> allowedTargets)
        : PhysicalItem(name, owner, false), allowedTargets(std::move(allowedTargets)) {}

    void use(Character* user, Character* target) override {
        if (!user || !user->isAlive()) {
            narrator.logEvent("Error: User is not alive or does not exist.");
            return;
        }
        if (!target || !target->isAlive()) {
            narrator.logEvent("Error: Target is not valid or not alive.");
            return;
        }
        auto it = find(allowedTargets.begin(), allowedTargets.end(), target);
        if (it == allowedTargets.end()) {
            narrator.logEvent(user->getName() + " attempted to cast " + name + " on an unauthorized target: " + target->getName() + ".");
            return;
        }
        narrator.logEvent(user->getName() + " casts " + name + " on " + target->getName() + ".");
    }

    void setup() override {
    }

    void print(ostream& os) const override {
        os << "Spell: " << name << endl;
    }
};
class Potion : public PhysicalItem {
    int healValue;

public:
    Potion(string name, Character* owner, int healValue)
        : PhysicalItem(name, owner, true) {
        if (healValue <= 0) {
            throw std::invalid_argument("Error caught: healValue must be positive.");
        }
        this->healValue = healValue;
    }

    void use(Character* user, Character* target) override {
        if (user && user->isAlive() && target && target->isAlive() && isUsableOnce) {
            target->heal(healValue);
            narrator.logEvent(user->getName() + " uses " + name + " on " + target->getName() + ", healing " + std::to_string(healValue) + " HP.");
            isUsableOnce = false;
        }
    }

    void setup() override {
    }

    void print(ostream& os) const override {
        os << "Potion: " << name << " HealValue: " << healValue << endl;
    }
};









class PotionUser {
public:
    virtual void drinkPotion(const string& potionName, Character* target) = 0;
    virtual void showPotions() const = 0;
    virtual ~PotionUser() {}
};



class Weapon : public PhysicalItem{
    int damage;

public:
    Weapon(string name, Character* owner, int damage)
        : PhysicalItem(name, owner, false) {
        if (damage <= 0) {
            throw std::invalid_argument("Error caught: damageValue must be positive.");
        }
        this->damage = damage;
    }

    void use(Character* user, Character* target) override {
        if (user && target) {
            target->takeDamage(damage);
            narrator.logEvent(user->getName() + " attacks " + target->getName() + " with " + name + ", dealing " + std::to_string(damage) + " damage.");
        }
    }

    void setup() override {}

    void print(ostream& os) const override {
        os << "Weapon: " << name << " Damage: " << damage << endl;
    }
};

class WeaponUser {
public:
    virtual void attack(Character* target, const string& weaponName) = 0;
    virtual void showWeapons() = 0;
    virtual ~WeaponUser() {}
};

class Fighter : public Character, public WeaponUser, public PotionUser {
protected:
    Container<Weapon> arsenal;
    Container<Potion> medicalBag;
public:

    Fighter(string name, int hp) : Character(name, hp), arsenal(3), medicalBag(5) {}
    bool canCarryWeapon() const override { return true; }
    bool canCarryPotion() const override { return true; }
    bool canCarrySpell() const override { return false; }
    std::string getType() const override {
        return "Fighter";}
    bool addItem(std::unique_ptr<PhysicalItem> item) override {
        if (typeid(*item) == typeid(Weapon) && !canCarryWeapon()) {
            narrator.logEvent("Error caught: " + getName() + " can't carry weapons.");
            return false;
        } else if (typeid(*item) == typeid(Potion) && !canCarryPotion()) {
            narrator.logEvent("Error caught: " + getName() + " can't carry potions.");
            return false;
        } else if (typeid(*item) == typeid(Spell) && !canCarrySpell()) {
            narrator.logEvent("Error caught: " + getName() + " can't carry spells.");
            return false;
        }

        if (auto weaponPtr = dynamic_cast<Weapon*>(item.get())) {
            std::unique_ptr<Weapon> weaponUniquePtr(weaponPtr);
            item.release();
            return arsenal.addItem(std::move(weaponUniquePtr));
        } else if (auto potionPtr = dynamic_cast<Potion*>(item.get())) {
            std::unique_ptr<Potion> potionUniquePtr(potionPtr);
            item.release();
            return medicalBag.addItem(std::move(potionUniquePtr));
        }

        narrator.logEvent("Error caught: Item type not supported for " + getName() + ".");
        return false;
    }
    void showPotions() const override{
        medicalBag.print();
    }
    void attack(Character* target, const string& weaponName) override {
        if (!this->isAlive()) {
            throw std::runtime_error(getName() + " is not alive to perform an attack.");
        }
        if (!target || !target->isAlive()) {
            throw std::runtime_error("Target is not valid or not alive.");
        }
        Weapon* weapon = arsenal.getItem(weaponName);
        if (!weapon) {
            throw std::runtime_error("Weapon " + weaponName + " not found in arsenal.");
        }
        weapon->use(this, target);
    }

    void showWeapons() override {
        arsenal.print();
    }
    void drinkPotion(const string& potionName, Character* target) override {
        if (!this->isAlive()) {
            throw std::runtime_error(getName() + " is not alive to drink a potion.");
        }
        Potion* potion = medicalBag.getItem(potionName);
        if (!potion) {
            throw std::runtime_error("Potion " + potionName + " not found in medical bag.");
        }
        potion->use(this, target);
        medicalBag.removeItem(potionName); // Ensure the potion is removed after use.
    }
    void print(ostream& os) const override {
        Character::print(os); // Use Character's print and then add additional details if needed
    }
};





class SpellUser {
public:
    virtual void castSpell(const string& spellName, Character* target) = 0;
    virtual void showSpells() const = 0;
    virtual ~SpellUser() {}
};

class Wizard : public Character, public SpellUser, public PotionUser{
    Container<Spell> spellBook;
    Container<Potion> medicalBag;

public:
    Wizard(string name, int hp) : Character(name, hp), spellBook(10), medicalBag(10) {}

    bool canCarryWeapon() const override { return false; }
    bool canCarryPotion() const override { return true; }
    bool canCarrySpell() const override { return true; }

    bool addItem(std::unique_ptr<PhysicalItem> item) override {
        // First, check if the character can carry the item based on its type.
        if (typeid(*item) == typeid(Weapon) && !canCarryWeapon()) {
            narrator.logEvent("Error caught: " + getName() + " can't carry weapons.");
            return false;
        } else if (typeid(*item) == typeid(Potion) && !canCarryPotion()) {
            narrator.logEvent("Error caught: " + getName() + " can't carry potions.");
            return false;
        } else if (typeid(*item) == typeid(Spell) && !canCarrySpell()) {
            narrator.logEvent("Error caught: " + getName() + " can't carry spells.");
            return false;
        }

        // Attempt to add the item to the appropriate container.
        if (typeid(*item) == typeid(Spell)) {
            // Release the ownership of the base pointer and cast it to the derived type.
            Spell* spellPtr = static_cast<Spell*>(item.release());
            // Take ownership with the correct unique_ptr type.
            std::unique_ptr<Spell> spellUniquePtr(spellPtr);
            // Add the item to the spellBook container.
            return spellBook.addItem(std::move(spellUniquePtr));
        }

        return false;
    }
    std::string getType() const override {
        return "Wizard";}



//    bool addItem(std::unique_ptr<PhysicalItem> item) override {
//        if (typeid(*item) == typeid(Weapon)) {
//            narrator.logEvent("Error caught: Wizard " + getName() + " can't have weapons.");
//            return false;
//        } else if (typeid(*item) == typeid(Potion)) {
//            auto potionPtr = static_cast<Potion*>(item.release());
//            std::unique_ptr<Potion> potionUniquePtr(potionPtr);
//            return medicalBag.addItem(std::move(potionUniquePtr));
//        } else if (typeid(*item) == typeid(Spell)) {
//            auto spellPtr = static_cast<Spell*>(item.release());
//            std::unique_ptr<Spell> spellUniquePtr(spellPtr);
//            return spellBook.addItem(std::move(spellUniquePtr));
//        }
//        narrator.logEvent("Error caught: Item type not supported for " + getName() + ".");
//        return false;
//    }

    void castSpell(const string& spellName, Character* target) override {
        Spell* spell = spellBook.getItem(spellName);
        if (spell) {
            spell->use(this, target);
            // Assuming the spell is used up and removed from the spell book
            spellBook.removeItem(spellName);
        }
    }

    void showSpells() const override {
        spellBook.print();
    }

    void drinkPotion(const string& potionName, Character* target) override {
        if (!this->isAlive()) {
            throw std::runtime_error(getName() + " is not alive to drink a potion.");
        }
        Potion* potion = medicalBag.getItem(potionName);
        if (!potion) {
            throw std::runtime_error("Potion " + potionName + " not found in medical bag.");
        }
        potion->use(this, target);
        medicalBag.removeItem(potionName); // Ensure the potion is removed after use.
    }

    void showPotions() const override{
        medicalBag.print();
    }

    // Additional methods as needed
};



class Archer : public Character, public WeaponUser, public SpellUser,  public PotionUser {
protected:
    Container<Weapon> arsenal;
    Container<Potion> medicalBag;
    Container<Spell> spellBook;

public:
    Archer(string name, int hp) : Character(name, hp), arsenal(2), medicalBag(3), spellBook(2) {}
    bool addItem(std::unique_ptr<PhysicalItem> item) override {
        if (typeid(*item) == typeid(Weapon) && !canCarryWeapon()) {
            narrator.logEvent("Error caught: " + getName() + " can't carry weapons.");
            return false;
        } else if (typeid(*item) == typeid(Potion) && !canCarryPotion()) {
            narrator.logEvent("Error caught: " + getName() + " can't carry potions.");
            return false;
        } else if (typeid(*item) == typeid(Spell) && !canCarrySpell()) {
            narrator.logEvent("Error caught: " + getName() + " can't carry spells.");
            return false;
        }

        if (auto weaponPtr = dynamic_cast<Weapon*>(item.get())) {
            std::unique_ptr<Weapon> weaponUniquePtr(weaponPtr);
            item.release(); // Prevent double deletion
            return arsenal.addItem(std::move(weaponUniquePtr));
        } else if (auto potionPtr = dynamic_cast<Potion*>(item.get())) {
            std::unique_ptr<Potion> potionUniquePtr(potionPtr);
            item.release(); // Prevent double deletion
            return medicalBag.addItem(std::move(potionUniquePtr));
        } else if (auto spellPtr = dynamic_cast<Spell*>(item.get())) {
            std::unique_ptr<Spell> spellUniquePtr(spellPtr);
            item.release(); // Prevent double deletion
            return spellBook.addItem(std::move(spellUniquePtr));
        }

        narrator.logEvent("Error caught: Item type not supported for " + getName() + ".");
        return false;
    }
//    bool addItem(std::unique_ptr<PhysicalItem> item) override {
//        if (typeid(*item) == typeid(Weapon)) {
//            auto weaponPtr = static_cast<Weapon*>(item.release()); // Release ownership and cast
//            std::unique_ptr<Weapon> weaponUniquePtr(weaponPtr); // Take ownership as the correct type
//            return arsenal.addItem(std::move(weaponUniquePtr));
//        } else if (typeid(*item) == typeid(Potion)) {
//            auto potionPtr = static_cast<Potion*>(item.release()); // Release and cast
//            std::unique_ptr<Potion> potionUniquePtr(potionPtr); // Take ownership as the correct type
//            return medicalBag.addItem(std::move(potionUniquePtr));
//        } else if (typeid(*item) == typeid(Spell)) {
//            auto spellPtr = static_cast<Spell*>(item.release()); // Release and cast
//            std::unique_ptr<Spell> spellUniquePtr(spellPtr); // Take ownership as the correct type
//            return spellBook.addItem(std::move(spellUniquePtr));
//        }
//        narrator.logEvent("Error caught: Item type not supported for Archer " + getName() + ".");
//        return false;
//    }
    std::string getType() const override {
        return "Archer";}
    void attack(Character* target, const string& weaponName) override {
        if (!target || !target->isAlive()) {
            narrator.logEvent("Error caught: " + getName() + " is not alive to perform an attack.");
            return;
        }
        Weapon* weapon = arsenal.getItem(weaponName);
        if (!weapon) {
            narrator.logEvent("Error caught: " + getName() + " doesn't own the weapon " + weaponName + ".");
            return;
        }
        weapon->use(this, target);
    }

    void showWeapons() override {
        arsenal.print();
    }

    void castSpell(const string& spellName, Character* target) override {
        if (this->isAlive()) {
            Spell* spell = spellBook.getItem(spellName);
            if (spell) {
                spell->use(this, target);
                spellBook.removeItem(spellName); // Remove the spell after use
            }
        }
    }

    void showSpells() const override {
        spellBook.print();
    }

    void drinkPotion(const string& potionName, Character* target) override {
        if (!this->isAlive()) {
            throw std::runtime_error(getName() + " is not alive to drink a potion.");
        }
        Potion* potion = medicalBag.getItem(potionName);
        if (!potion) {
            throw std::runtime_error("Potion " + potionName + " not found in medical bag.");
        }
        potion->use(this, target);
        medicalBag.removeItem(potionName); // Ensure the potion is removed after use.
    }

    void showPotions() const override {
        medicalBag.print();
    }

};





//сделать массив имен и обращаться к объекту по имени перса 2) сделать проверку в weapon user


unordered_map<string, unique_ptr<Character>> characters;
void processEvent(const string& event) {
    istringstream iss(event);
    string eventType;
    iss >> eventType;

    if (eventType == "Create") {
        string itemType;
        iss >> itemType;
        if (itemType == "character") {
            string type, name;
            int initHP;
            iss >> type >> name >> initHP;
            if (type == "fighter") {
                characters[name] = make_unique<Fighter>(name, initHP);
            } else if (type == "wizard") {
                characters[name] = make_unique<Wizard>(name, initHP);
            } else if (type == "archer") {
                characters[name] = make_unique<Archer>(name, initHP);
            }
            cout << "A new " << type << " came to town, " << name << "." << endl;
        } else if (itemType == "item") {
            string itemName, ownerName, itemNameSpecific;
            iss >> itemName;
            if (itemName == "weapon") {
                int damageValue;
                iss >> ownerName >> itemNameSpecific >> damageValue;
                if (characters.find(ownerName) != characters.end()) {
                    characters[ownerName]->addItem(make_unique<Weapon>(itemNameSpecific, characters[ownerName].get(), damageValue));
                    cout << ownerName << " just obtained a new weapon called " << itemNameSpecific << "." << endl;
                }
            } else if (itemName == "potion") {
                int healValue;
                iss >> ownerName >> itemNameSpecific >> healValue;
                if (characters.find(ownerName) != characters.end()) {
                    characters[ownerName]->addItem(make_unique<Potion>(itemNameSpecific, characters[ownerName].get(), healValue));
                    cout << ownerName << " just obtained a new potion called " << itemNameSpecific << "." << endl;
                }
            } else if (itemName == "spell") {
                int m;
                iss >> ownerName >> itemNameSpecific >> m;
                vector<Character*> allowedTargets;
                for (int i = 0; i < m; ++i) {
                    string targetName;
                    iss >> targetName;
                    if (characters.find(targetName) != characters.end()) {
                        allowedTargets.push_back(characters[targetName].get());
                    }
                }
                if (characters.find(ownerName) != characters.end()) {
                    characters[ownerName]->addItem(make_unique<Spell>(itemNameSpecific, characters[ownerName].get(), allowedTargets));
                    cout << ownerName << " just obtained a new spell called " << itemNameSpecific << "." << endl;
                }
            }
        }
    } else if (eventType == "Attack") {
        string attackerName, targetName, weaponName;
        iss >> attackerName >> targetName >> weaponName;
        if (characters.find(attackerName) != characters.end() && characters.find(targetName) != characters.end()) {
            dynamic_cast<WeaponUser*>(characters[attackerName].get())->attack(characters[targetName].get(), weaponName);
            cout << attackerName << " attacks " << targetName << " with their " << weaponName << "!" << endl;
        }
    } else if (eventType == "Cast") {
        string casterName, targetName, spellName;
        iss >> casterName >> targetName >> spellName;
        if (characters.find(casterName) != characters.end() && characters.find(targetName) != characters.end()) {
            dynamic_cast<SpellUser*>(characters[casterName].get())->castSpell(spellName, characters[targetName].get());
            cout << casterName << " casts " << spellName << " on " << targetName << "!" << endl;
        }
    } else if (eventType == "Drink") {
        string supplierName, drinkerName, potionName;
        iss >> supplierName >> drinkerName >> potionName;
        if (characters.find(drinkerName) != characters.end()) {
            dynamic_cast<PotionUser*>(characters[drinkerName].get())->drinkPotion(potionName, characters[drinkerName].get());
            cout << drinkerName << " drinks " << potionName << " from " << supplierName << "." << endl;
        }
    }
        // Continue from the previous implementation
    else if (eventType == "Dialogue") {
        string speaker;
        int sp_len;
        iss >> speaker >> sp_len;
        string speech, word;
        // Skip the initial space before the speech starts.
        iss.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
        for (int i = 0; i < sp_len; ++i) {
            iss >> word;
            speech += word;
            if (i < sp_len - 1) {
                speech += " "; // Add space between words, but not after the last word.
            }
        }
        cout << speaker << ": " << speech << endl;
    }
    else if (eventType == "Show") {
        string showType;
        iss >> showType;
        if (showType == "characters") {
            // Assuming characters are stored in an unordered_map and need sorting for display
            vector<string> sortedNames;
            if (showType == "characters") {
                vector<string> sortedNames;
                for (const auto& pair : characters) {
                    if (pair.second->isAlive()) {
                        sortedNames.push_back(pair.first + ":" + pair.second->getType() + ":" + to_string(pair.second->getHP()));
                    }
                }
                sort(sortedNames.begin(), sortedNames.end());
                for (const auto& name : sortedNames) {
                    cout << name << " ";
                }
                cout << endl;
            }
        }
        else if (showType == "weapons" || showType == "potions" || showType == "spells") {
            string characterName;
            iss >> characterName;
            if (characters.find(characterName) != characters.end()) {
                if (showType == "weapons") {
                    dynamic_cast<WeaponUser*>(characters[characterName].get())->showWeapons();
                } else if (showType == "potions") {
                    dynamic_cast<PotionUser*>(characters[characterName].get())->showPotions();
                } else if (showType == "spells") {
                    dynamic_cast<SpellUser*>(characters[characterName].get())->showSpells();
                }
            }
        }
    }
}



int main() {
    string line;
    while (getline(cin, line)) {
        processEvent(line);
    }
    return 0;
}