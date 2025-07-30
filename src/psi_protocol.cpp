#include "psi_protocol.h"
#include <iostream>
#include <algorithm>
#include <stdexcept>

namespace SecretSharingPSI {

PSIProtocol::PSIProtocol(Role role, const Config& config) 
    : role_(role), config_(config), initialized_(false) {
    
    // 初始化统计信息
    stats_ = Statistics{};
}

PSIProtocol::~PSIProtocol() = default;

bool PSIProtocol::initialize() {
    try {
        // 初始化秘密分享
        secret_sharing_ = std::make_unique<SecretSharing>();
        
        // 初始化OPRF
        if (!initializeOPRF()) {
            return false;
        }
        
        // 初始化哈希表
        if (!initializeHashTables()) {
            return false;
        }
        
        initialized_ = true;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "PSI Protocol initialization failed: " << e.what() << std::endl;
        return false;
    }
}

bool PSIProtocol::generateSeeds(Seed& seed_a, Seed& seed_b) {
    if (role_ != Role::TEE) {
        std::cerr << "Only TEE can generate seeds" << std::endl;
        return false;
    }
    
    return secret_sharing_->generateSeeds(seed_a, seed_b);
}

SecretSharing::PartyBData PSIProtocol::generatePartyBData(const Seed& seed_a, const Seed& seed_b) {
    if (role_ != Role::TEE) {
        throw std::runtime_error("Only TEE can generate party B data");
    }
    
    return secret_sharing_->generateDataForB(seed_a, seed_b, config_.beaver_triple_count);
}

bool PSIProtocol::initializeSecretSharing(const Seed& seed, 
                                         const SecretSharing::PartyBData* party_b_data) {
    if (!validateState("initialized")) {
        return false;
    }
    
    try {
        if (role_ == Role::SENDER) {
            secret_sharing_->initializePartyA(seed, config_.beaver_triple_count);
        } else if (role_ == Role::RECEIVER) {
            if (!party_b_data) {
                std::cerr << "Party B requires pre-processed data" << std::endl;
                return false;
            }
            secret_sharing_->initializePartyB(seed, party_b_data->pairs, party_b_data->c1_values);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Secret sharing initialization failed: " << e.what() << std::endl;
        return false;
    }
}

bool PSIProtocol::setInputSet(const std::vector<Item>& items) {
    if (!validateState("initialized")) {
        return false;
    }
    
    input_items_ = items;
    stats_.input_set_size = items.size();
    return true;
}

bool PSIProtocol::loadInputSet(const std::string& filename) {
    if (!validateState("initialized")) {
        return false;
    }
    
    try {
        input_items_ = Item::loadFromFile(filename);
        stats_.input_set_size = input_items_.size();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to load input set from file: " << e.what() << std::endl;
        return false;
    }
}

bool PSIProtocol::processOPRF() {
    if (!validateState("has input set")) {
        return false;
    }
    
    if (!oprf_->isInitialized()) {
        std::cerr << "OPRF not initialized" << std::endl;
        return false;
    }
    
    try {
        oprf_items_ = oprf_->batchProcess(input_items_);
        stats_.oprf_processed_count = oprf_items_.size();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "OPRF processing failed: " << e.what() << std::endl;
        return false;
    }
}

bool PSIProtocol::buildHashTable() {
    if (oprf_items_.empty()) {
        std::cerr << "No OPRF items to build hash table" << std::endl;
        return false;
    }
    
    try {
        if (role_ == Role::SENDER) {
            // 发送方使用布谷鸟哈希表
            if (!cuckoo_table_) {
                std::cerr << "Cuckoo hash table not initialized" << std::endl;
                return false;
            }
            
            size_t inserted = cuckoo_table_->batchInsert(oprf_items_);
            stats_.hash_table_size = inserted;
            stats_.hash_table_load_factor = cuckoo_table_->getCurrentLoadFactor();
            
            std::cout << "Sender: Inserted " << inserted << " items into cuckoo hash table" << std::endl;
            
        } else if (role_ == Role::RECEIVER) {
            // 接收方使用普通哈希表
            if (!hash_table_) {
                std::cerr << "Hash table not initialized" << std::endl;
                return false;
            }
            
            size_t inserted = hash_table_->batchInsert(oprf_items_);
            stats_.hash_table_size = inserted;
            stats_.hash_table_load_factor = hash_table_->getCurrentLoadFactor();
            
            std::cout << "Receiver: Inserted " << inserted << " items into hash table" << std::endl;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Hash table construction failed: " << e.what() << std::endl;
        return false;
    }
}

std::vector<uint64_t> PSIProtocol::getSenderQueries() const {
    if (role_ != Role::SENDER) {
        throw std::runtime_error("Only sender can generate queries");
    }
    
    std::vector<uint64_t> queries;
    queries.reserve(oprf_items_.size());
    
    for (const auto& item : oprf_items_) {
        queries.push_back(item.hash_value);
    }
    
    return queries;
}

std::vector<std::pair<uint64_t, uint64_t>> PSIProtocol::processQueries(const std::vector<uint64_t>& queries) {
    if (role_ != Role::RECEIVER) {
        throw std::runtime_error("Only receiver can process queries");
    }
    
    if (!hash_table_) {
        throw std::runtime_error("Hash table not initialized");
    }
    
    return hash_table_->batchFind(queries);
}

PSIResult PSIProtocol::computeIntersection(const std::vector<std::pair<uint64_t, uint64_t>>& query_results) {
    if (role_ != Role::SENDER) {
        throw std::runtime_error("Only sender can compute intersection");
    }
    
    PSIResult result;
    result.sender_set_size = input_items_.size();
    result.receiver_set_size = 0;  // 接收方大小未知
    
    // 查找匹配的项目
    for (const auto& query_result : query_results) {
        uint64_t query_key = query_result.first;
        
        // 在发送方的OPRF项目中查找匹配
        for (const auto& oprf_item : oprf_items_) {
            if (oprf_item.hash_value == query_key) {
                result.intersection.push_back(oprf_item.original_item);
                break;
            }
        }
    }
    
    result.intersection_size = result.intersection.size();
    stats_.intersection_size = result.intersection_size;
    
    return result;
}

PSIResult PSIProtocol::runProtocol(PSIProtocol& other_protocol) {
    if (role_ == other_protocol.role_) {
        throw std::runtime_error("Both protocols have the same role");
    }
    
    PSIResult result;
    
    try {
        // 确保两个协议都已处理OPRF和构建哈希表
        if (!processOPRF() || !other_protocol.processOPRF()) {
            throw std::runtime_error("OPRF processing failed");
        }
        
        if (!buildHashTable() || !other_protocol.buildHashTable()) {
            throw std::runtime_error("Hash table construction failed");
        }
        
        if (role_ == Role::SENDER) {
            // 发送方生成查询
            auto queries = getSenderQueries();
            
            // 接收方处理查询
            auto query_results = other_protocol.processQueries(queries);
            
            // 发送方计算交集
            result = computeIntersection(query_results);
            result.receiver_set_size = other_protocol.stats_.input_set_size;
            
        } else if (role_ == Role::RECEIVER) {
            // 接收方生成查询
            auto queries = other_protocol.getSenderQueries();
            
            // 发送方处理查询
            auto query_results = processQueries(queries);
            
            // 发送方计算交集
            result = other_protocol.computeIntersection(query_results);
            result.receiver_set_size = stats_.input_set_size;
        }
        
        updateStatistics();
        other_protocol.updateStatistics();
        
        return result;
        
    } catch (const std::exception& e) {
        std::cerr << "Protocol execution failed: " << e.what() << std::endl;
        return PSIResult{};
    }
}

PSIProtocol::Statistics PSIProtocol::getStatistics() const {
    updateStatistics();
    return stats_;
}

PSIProtocol::Role PSIProtocol::getRole() const {
    return role_;
}

bool PSIProtocol::isInitialized() const {
    return initialized_;
}

bool PSIProtocol::initializeOPRF() {
    oprf_ = std::make_unique<OPRF>();
    return oprf_->generateKey();
}

bool PSIProtocol::initializeHashTables() {
    if (role_ == Role::SENDER || role_ == Role::TEE) {
        cuckoo_table_ = std::make_unique<CuckooHashTable>(
            config_.hash_table_capacity, 
            config_.cuckoo_max_iterations
        );
    }
    
    if (role_ == Role::RECEIVER || role_ == Role::TEE) {
        hash_table_ = std::make_unique<HashTable>(
            config_.hash_table_capacity,
            config_.hash_table_load_factor
        );
    }
    
    return true;
}

void PSIProtocol::updateStatistics() const {
    if (hash_table_) {
        auto hash_stats = hash_table_->getStatistics();
        stats_.hash_table_size = hash_stats.element_count;
        stats_.hash_table_load_factor = hash_stats.load_factor;
    }
    
    if (cuckoo_table_) {
        auto cuckoo_stats = cuckoo_table_->getStatistics();
        stats_.hash_table_size = cuckoo_stats.element_count;
        stats_.hash_table_load_factor = cuckoo_stats.load_factor;
    }
}

bool PSIProtocol::validateState(const std::string& required_state) const {
    if (required_state == "initialized") {
        return initialized_;
    } else if (required_state == "has input set") {
        return initialized_ && !input_items_.empty();
    } else if (required_state == "oprf processed") {
        return initialized_ && !input_items_.empty() && !oprf_items_.empty();
    }
    
    return false;
}

} // namespace SecretSharingPSI