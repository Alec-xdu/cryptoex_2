#include "../include/beaver_triple.h"
#include <random>
#include <iomanip>

namespace SecretShare {

void BeaverTripleUtils::printTriple(const BeaverTriple& triple, const std::string& party_name) {
    std::cout << party_name << " Triple: [a=" << triple.a 
              << ", b=" << triple.b 
              << ", c=" << triple.c << "]" << std::endl;
}

bool BeaverTripleUtils::verifyTriples(const BeaverTriple& triple_a, const BeaverTriple& triple_b) {
    // 重建原始值
    Share a = triple_a.a + triple_b.a;
    Share b = triple_a.b + triple_b.b;
    Share c = triple_a.c + triple_b.c;
    
    // 验证 c = a * b
    return (c == a * b);
}

bool BeaverTripleUtils::verifyBatchTriples(const std::vector<BeaverTriple>& triples_a, 
                                          const std::vector<BeaverTriple>& triples_b) {
    if (triples_a.size() != triples_b.size()) {
        return false;
    }
    
    for (size_t i = 0; i < triples_a.size(); ++i) {
        if (!verifyTriples(triples_a[i], triples_b[i])) {
            return false;
        }
    }
    
    return true;
}

void BeaverTripleUtils::generateTestShares(Share secret, Share& share_a, Share& share_b) {
    // 使用随机数生成器创建分享
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<Share> dis;
    
    share_a = dis(gen);
    share_b = secret - share_a;  // 确保 share_a + share_b = secret
}

void BeaverTripleUtils::generateBatchTestShares(const std::vector<Share>& secrets,
                                               std::vector<Share>& shares_a,
                                               std::vector<Share>& shares_b) {
    shares_a.clear();
    shares_b.clear();
    shares_a.reserve(secrets.size());
    shares_b.reserve(secrets.size());
    
    for (Share secret : secrets) {
        Share share_a, share_b;
        generateTestShares(secret, share_a, share_b);
        shares_a.push_back(share_a);
        shares_b.push_back(share_b);
    }
}

} // namespace SecretShare