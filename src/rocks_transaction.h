// rocks_transaction.h

/**
 *    Copyright (C) 2014 MongoDB Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#pragma once

#include <atomic>
#include <set>
#include <memory>
#include <string>

#include <boost/thread/mutex.hpp>

namespace mongo {
    class RocksTransaction;

    class RocksTransactionEngine {
    public:
        RocksTransactionEngine();

        uint64_t getLatestSeqId() const {
            return _latestSeqId.load(std::memory_order::memory_order_acquire);
        }

    private:
        uint64_t nextTransactionId() {
          return _nextTransactionId.fetch_add(1);
        }

        friend class RocksTransaction;
        std::atomic<uint64_t> _latestSeqId;
        std::atomic<uint64_t> _nextTransactionId;
        // Lock when mutating state here
        boost::mutex _commitLock;

        static const size_t kNumSeqIdShards = 1 << 20;
        // Slots to store latest update SeqID for documents.
        uint64_t _seqId[kNumSeqIdShards];
        uint64_t _uncommittedTransactionId[kNumSeqIdShards];
    };

    class RocksTransaction {
    public:
        RocksTransaction(RocksTransactionEngine* transactionEngine)
            : _snapshotSeqId(std::numeric_limits<uint64_t>::max()),
              _transactionId(transactionEngine->nextTransactionId()),
              _transactionEngine(transactionEngine) {}

        ~RocksTransaction() { abort(); }

        // returns true if OK
        // returns false on conflict
        bool registerWrite(uint64_t hash) ;

        void commit();

        void abort();

        void recordSnapshotId();

    private:
        friend class RocksTransactionEngine;
        uint64_t _snapshotSeqId;
        uint64_t _transactionId;
        RocksTransactionEngine* _transactionEngine;
        std::set<uint64_t> _writeShards;
    };
}
