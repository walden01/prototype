/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file Eth.cpp
 * @authors:
 *   Gav Wood <i@gavwood.com>
 *   Marek Kotewicz <marek@ethdev.com>
 * @date 2014
 */

#include <csignal>
#include <jsonrpccpp/common/exception.h>
#include <libdevcore/CommonData.h>
#include <libethereum/Client.h>
#include <libethashseal/EthashClient.h>
#include <libwebthree/WebThree.h>
#include <libethcore/CommonJS.h>
#include <libweb3jsonrpc/JsonHelper.h>
#include "Eth.h"
#include "AccountHolder.h"
#include "../libevm/Vote.h"
#include <utils/json_spirit/json_spirit_value.h>
#include <utils/json_spirit/json_spirit_reader_template.h>
#include <utils/json_spirit/json_spirit_writer_template.h>

using namespace std;
using namespace jsonrpc;
using namespace dev;
using namespace eth;
using namespace shh;
using namespace dev::rpc;

Eth::Eth(eth::Interface& _eth, eth::AccountHolder& _ethAccounts):
	m_eth(_eth),
	m_ethAccounts(_ethAccounts)
{
}

string Eth::eth_protocolVersion()
{
	return toJS(eth::c_protocolVersion);
}

string Eth::eth_coinbase()
{
	return toJS(client()->author());
}

string Eth::eth_hashrate()
{
	try
	{
		return toJS(asEthashClient(client())->hashrate());
	}
	catch (InvalidSealEngine&)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

bool Eth::eth_mining()
{
	try
	{
		return asEthashClient(client())->isMining();
	}
	catch (InvalidSealEngine&)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_gasPrice()
{
	return toJS(client()->gasBidPrice());
}

Json::Value Eth::eth_accounts()
{
	return toJson(m_ethAccounts.allAccounts());
}

string Eth::eth_blockNumber()
{
	return toJS(client()->number());
}


string Eth::eth_getBalance(string const& _address, string const& _blockNumber)
{
	try
	{
		return toJS(client()->balanceAt(jsToAddress(_address), jsToBlockNumber(_blockNumber)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_getStorageAt(string const& _address, string const& _position, string const& _blockNumber)
{
	try
	{
		return toJS(toCompactBigEndian(client()->stateAt(jsToAddress(_address), jsToU256(_position), jsToBlockNumber(_blockNumber)), 32));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_getStorageRoot(string const& _address, string const& _blockNumber)
{
	try
	{
		return toString(client()->stateRootAt(jsToAddress(_address), jsToBlockNumber(_blockNumber)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_pendingTransactions()
{
	//Return list of transaction that being sent by local accounts
	Transactions ours;
	for (Transaction const& pending:client()->pending())
	{
		for (Address const& account:m_ethAccounts.allAccounts())
		{
			if (pending.sender() == account)
			{
				ours.push_back(pending);
				break;
			}
		}
	}

	return toJS(ours);
}

string Eth::eth_getTransactionCount(string const& _address, string const& _blockNumber)
{
	try
	{
		return toJS(client()->countAt(jsToAddress(_address), jsToBlockNumber(_blockNumber)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getBlockTransactionCountByHash(string const& _blockHash)
{
	try
	{
		h256 blockHash = jsToFixed<32>(_blockHash);
		if (!client()->isKnown(blockHash))
			return Json::Value(Json::nullValue);

		return toJS(client()->transactionCount(blockHash));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getBlockTransactionCountByNumber(string const& _blockNumber)
{
	try
	{
		BlockNumber blockNumber = jsToBlockNumber(_blockNumber);
		if (!client()->isKnown(blockNumber))
			return Json::Value(Json::nullValue);

		return toJS(client()->transactionCount(jsToBlockNumber(_blockNumber)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getUncleCountByBlockHash(string const& _blockHash)
{
	try
	{
		h256 blockHash = jsToFixed<32>(_blockHash);
		if (!client()->isKnown(blockHash))
			return Json::Value(Json::nullValue);

		return toJS(client()->uncleCount(blockHash));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getUncleCountByBlockNumber(string const& _blockNumber)
{
	try
	{
		BlockNumber blockNumber = jsToBlockNumber(_blockNumber);
		if (!client()->isKnown(blockNumber))
			return Json::Value(Json::nullValue);

		return toJS(client()->uncleCount(blockNumber));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_getCode(string const& _address, string const& _blockNumber)
{
	try
	{
		return toJS(client()->codeAt(jsToAddress(_address), jsToBlockNumber(_blockNumber)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

void Eth::setTransactionDefaults(TransactionSkeleton& _t)
{
	if (!_t.from)
		_t.from = m_ethAccounts.defaultTransactAccount();
}

string Eth::eth_sendTransaction(Json::Value const& _json)
{
	try
	{
		TransactionSkeleton t = toTransactionSkeleton(_json);
		setTransactionDefaults(t);
		TransactionNotification n = m_ethAccounts.authenticate(t);
		switch (n.r)
		{
		case TransactionRepercussion::Success:
			return toJS(n.hash);
		case TransactionRepercussion::ProxySuccess:
			return toJS(n.hash);// TODO: give back something more useful than an empty hash.
		case TransactionRepercussion::UnknownAccount:
			BOOST_THROW_EXCEPTION(JsonRpcException("Account unknown."));
		case TransactionRepercussion::Locked:
			BOOST_THROW_EXCEPTION(JsonRpcException("Account is locked."));
		case TransactionRepercussion::Refused:
			BOOST_THROW_EXCEPTION(JsonRpcException("Transaction rejected by user."));
		case TransactionRepercussion::Unknown:
			BOOST_THROW_EXCEPTION(JsonRpcException("Unknown reason."));
		}
	}
	catch (JsonRpcException&)
	{
		throw;
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
	BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	return string();
}

string Eth::eth_signTransaction(Json::Value const& _json)
{
	try
	{
		TransactionSkeleton t = toTransactionSkeleton(_json);
		setTransactionDefaults(t);
		TransactionNotification n = m_ethAccounts.authenticate(t);
		switch (n.r)
		{
		case TransactionRepercussion::Success:
			return toJS(n.hash);
		case TransactionRepercussion::ProxySuccess:
			return toJS(n.hash);// TODO: give back something more useful than an empty hash.
		default:
			// TODO: provide more useful information in the exception.
			BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
		}
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_inspectTransaction(std::string const& _rlp)
{
	try
	{
		return toJson(Transaction(jsToBytes(_rlp, OnFailed::Throw), CheckTransaction::Everything));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_sendRawTransaction(std::string const& _rlp)
{
	try
	{
		if (client()->injectTransaction(jsToBytes(_rlp, OnFailed::Throw)) == ImportResult::Success)
		{
			Transaction tx(jsToBytes(_rlp, OnFailed::Throw), CheckTransaction::None);
			return toJS(tx.sha3());
		}
		else
			return toJS(h256());
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_call(Json::Value const& _json, string const& _blockNumber)
{
	try
	{
		TransactionSkeleton t = toTransactionSkeleton(_json);
		setTransactionDefaults(t);
		ExecutionResult er = client()->call(t.from, t.value, t.to, t.data, t.gas, t.gasPrice, jsToBlockNumber(_blockNumber), FudgeFactor::Lenient);
		return toJS(er.output);
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_estimateGas(Json::Value const& _json)
{
	try
	{
		TransactionSkeleton t = toTransactionSkeleton(_json);
		setTransactionDefaults(t);
		int64_t gas = static_cast<int64_t>(t.gas);
		return toJS(client()->estimateGas(t.from, t.value, t.to, t.data, gas, t.gasPrice, PendingBlock).first);
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

bool Eth::eth_flush()
{
	client()->flushTransactions();
	return true;
}

Json::Value Eth::eth_getBlockByHash(string const& _blockHash, bool _includeTransactions)
{
	try
	{
		h256 h = jsToFixed<32>(_blockHash);
		if (!client()->isKnown(h))
			return Json::Value(Json::nullValue);

		if (_includeTransactions)
			return toJson(client()->blockInfo(h), client()->blockDetails(h), client()->uncleHashes(h), client()->transactions(h), client()->sealEngine());
		else
			return toJson(client()->blockInfo(h), client()->blockDetails(h), client()->uncleHashes(h), client()->transactionHashes(h), client()->sealEngine());
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getBlockByNumber(string const& _blockNumber, bool _includeTransactions)
{
	try
	{
		BlockNumber h = jsToBlockNumber(_blockNumber);
		if (!client()->isKnown(h))
			return Json::Value(Json::nullValue);

		if (_includeTransactions)
			return toJson(client()->blockInfo(h), client()->blockDetails(h), client()->uncleHashes(h), client()->transactions(h), client()->sealEngine());
		else
			return toJson(client()->blockInfo(h), client()->blockDetails(h), client()->uncleHashes(h), client()->transactionHashes(h), client()->sealEngine());
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getTransactionByHash(string const& _transactionHash)
{
	try
	{
		h256 h = jsToFixed<32>(_transactionHash);
		if (!client()->isKnownTransaction(h))
			return Json::Value(Json::nullValue);

		return toJson(client()->localisedTransaction(h));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getTransactionByBlockHashAndIndex(string const& _blockHash, string const& _transactionIndex)
{
	try
	{
		h256 bh = jsToFixed<32>(_blockHash);
		unsigned ti = jsToInt(_transactionIndex);
		if (!client()->isKnownTransaction(bh, ti))
			return Json::Value(Json::nullValue);

		return toJson(client()->localisedTransaction(bh, ti));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getTransactionByBlockNumberAndIndex(string const& _blockNumber, string const& _transactionIndex)
{
	try
	{
		BlockNumber bn = jsToBlockNumber(_blockNumber);
		h256 bh = client()->hashFromNumber(bn);
		unsigned ti = jsToInt(_transactionIndex);
		if (!client()->isKnownTransaction(bh, ti))
			return Json::Value(Json::nullValue);

		return toJson(client()->localisedTransaction(bh, ti));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getTransactionReceipt(string const& _transactionHash)
{
	try
	{
		h256 h = jsToFixed<32>(_transactionHash);
		if (!client()->isKnownTransaction(h))
			return Json::Value(Json::nullValue);

		return toJson(client()->localisedTransactionReceipt(h));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getUncleByBlockHashAndIndex(string const& _blockHash, string const& _uncleIndex)
{
	try
	{
		return toJson(client()->uncle(jsToFixed<32>(_blockHash), jsToInt(_uncleIndex)), client()->sealEngine());
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getUncleByBlockNumberAndIndex(string const& _blockNumber, string const& _uncleIndex)
{
	try
	{
		return toJson(client()->uncle(jsToBlockNumber(_blockNumber), jsToInt(_uncleIndex)), client()->sealEngine());
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_newFilter(Json::Value const& _json)
{
	try
	{
		return toJS(client()->installWatch(toLogFilter(_json, *client())));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_newFilterEx(Json::Value const& _json)
{
	try
	{
		return toJS(client()->installWatch(toLogFilter(_json)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_newBlockFilter()
{
	h256 filter = dev::eth::ChainChangedFilter;
	return toJS(client()->installWatch(filter));
}

string Eth::eth_newPendingTransactionFilter()
{
	h256 filter = dev::eth::PendingChangedFilter;
	return toJS(client()->installWatch(filter));
}

bool Eth::eth_uninstallFilter(string const& _filterId)
{
	try
	{
		return client()->uninstallWatch(jsToInt(_filterId));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getFilterChanges(string const& _filterId)
{
	try
	{
		int id = jsToInt(_filterId);
		auto entries = client()->checkWatch(id);
//		if (entries.size())
//			cnote << "FIRING WATCH" << id << entries.size();
		return toJson(entries);
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getFilterChangesEx(string const& _filterId)
{
	try
	{
		int id = jsToInt(_filterId);
		auto entries = client()->checkWatch(id);
//		if (entries.size())
//			cnote << "FIRING WATCH" << id << entries.size();
		return toJsonByBlock(entries);
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getFilterLogs(string const& _filterId)
{
	try
	{
		return toJson(client()->logs(jsToInt(_filterId)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getFilterLogsEx(string const& _filterId)
{
	try
	{
		return toJsonByBlock(client()->logs(jsToInt(_filterId)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getLogs(Json::Value const& _json)
{
	try
	{
		return toJson(client()->logs(toLogFilter(_json, *client())));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getLogsEx(Json::Value const& _json)
{
	try
	{
		return toJsonByBlock(client()->logs(toLogFilter(_json)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getWork()
{
	try
	{
		Json::Value ret(Json::arrayValue);
		auto r = asEthashClient(client())->getEthashWork();
		ret.append(toJS(get<0>(r)));
		ret.append(toJS(get<1>(r)));
		ret.append(toJS(get<2>(r)));
		return ret;
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_syncing()
{
	dev::eth::SyncStatus sync = client()->syncStatus();
	if (sync.state == SyncState::Idle || !sync.majorSyncing)
		return Json::Value(false);

	Json::Value info(Json::objectValue);
	info["startingBlock"] = sync.startBlockNumber;
	info["highestBlock"] = sync.highestBlockNumber;
	info["currentBlock"] = sync.currentBlockNumber;
	return info;
}

Json::Value Eth::eth_getVote()
{
	try
	{
		
		string _blockNumber = "latest";		
		map<h256, pair<u256, u256>> storageMap = client()->storageAt(VoteInfoAddress, jsToBlockNumber(_blockNumber));
		std::unordered_map<u256, u256> voteMapChange;
		unordered_map<Address, VoteBace> returnMap;

		dev::bytes bytes0(12, '\0');
		for (auto iterator = storageMap.begin(); iterator != storageMap.end(); iterator++)
		{
			//dev::u256 iteratorFirst = iterator->first;
			//dev::bytes iteratorBytes = ((dev::h256)iteratorFirst).asBytes();
			//dev::bytes iteratorAddress(iteratorBytes.begin(), iteratorBytes.begin() + sizeof(dev::Address));
			//dev::bytes iteratorExpand(iteratorBytes.begin() + sizeof(dev::Address), iteratorBytes.end());
			//hashkey:h256->(key:u256,value:u256)
			dev::u256 key = iterator->second.first;
			dev::bytes iteratorBytes = ((dev::h256)key).asBytes();
			dev::bytes iteratorAddress(iteratorBytes.begin(), iteratorBytes.begin() + sizeof(dev::Address));
			dev::bytes iteratorExpand(iteratorBytes.begin() + sizeof(dev::Address), iteratorBytes.end());

			if (iteratorExpand == bytes0)
			{
				Vote reset(storageMap, voteMapChange, dev::Address(iteratorAddress));
				reset.load();
				VoteBace vb;
				vb.m_address = dev::Address(iteratorAddress);
				vb.m_assignNumber = reset.getAssignNumber();
				vb.m_isCandidate = reset.getIsCandidate();
				vb.m_isVoted = reset.getIsVoted();
				vb.m_receivedVoteNumber = reset.getReceivedVoteNumber();
				vb.m_unAssignNumber = reset.getUnAssignNumber();
				vb.m_votedNumber = reset.getVotedNumber();
				vb.m_voteTo = reset.getVoteTo();
				returnMap[dev::Address(iteratorAddress)] = vb;
			}
		}
		Json::Value res(Json::objectValue);
		for (auto i : returnMap)
		{
			Json::Value subRes(Json::objectValue);
			subRes["assignNumber"] = to_string(i.second.m_assignNumber);
			subRes["isCandidate"] = i.second.m_isCandidate;
			subRes["isVoted"] = i.second.m_isVoted;
			subRes["receivedVoteNumber"] = to_string(i.second.m_receivedVoteNumber);
			subRes["unAssignNumber"] = to_string(i.second.m_unAssignNumber);
			subRes["votedNumber"] = to_string(i.second.m_votedNumber);
			subRes["voteTo"] = i.second.m_voteTo.hex();

			res["0x" + i.first.hex()] = subRes;
		}
		return res;
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

bool Eth::eth_submitWork(string const& _nonce, string const&, string const& _mixHash)
{
	try
	{
		return asEthashClient(client())->submitEthashWork(jsToFixed<32>(_mixHash), jsToFixed<Nonce::size>(_nonce));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

bool Eth::eth_submitHashrate(string const& _hashes, string const& _id)
{
	try
	{
		asEthashClient(client())->submitExternalHashrate(jsToInt<32>(_hashes), jsToFixed<32>(_id));
		return true;
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_register(string const& _address)
{
	try
	{
		return toJS(m_ethAccounts.addProxyAccount(jsToAddress(_address)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

bool Eth::eth_unregister(string const& _accountId)
{
	try
	{
		return m_ethAccounts.removeProxyAccount(jsToInt(_accountId));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_fetchQueuedTransactions(string const& _accountId)
{
	try
	{
		auto id = jsToInt(_accountId);
		Json::Value ret(Json::arrayValue);
		// TODO: throw an error on no account with given id
		for (TransactionSkeleton const& t: m_ethAccounts.queuedTransactions(id))
			ret.append(toJson(t));
		m_ethAccounts.clearQueue(id);
		return ret;
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_makeKeys(string const& _a)
{
	try
	{
		int count = std::stoi(_a);
		if (count <= 0)
		{
			count = 100;
		}

		json_spirit::mValue v;
		string s("{""keys"":{}}");
		json_spirit::read_string(s, v);
		json_spirit::mObject& m = v.get_obj();
		for (int i = 0; i < count; i++)
		{
			KeyPair k(Secret::random());
			m[k.address().hex()] = toHex(k.secret().ref());
		}
		string filePath(boost::filesystem::current_path().string());
		writeFile(filePath + "/address-keys.json", asBytes(json_spirit::write_string(v, true)));

		return toJS(count);
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_testSend1(string const& _a)
{
	try
	{
		struct myAccount {
			string address;
			string secret;
		};
		std::vector<myAccount> as;
		string filePath(boost::filesystem::current_path().string());
		string s = contentsString(filePath + "/address-keys.json");
		json_spirit::mValue v;
		json_spirit::read_string(s, v);
		json_spirit::mObject keys = v.get_obj();
		for (const auto& key : keys)
		{
			myAccount account{ key.first , key.second.get_str() };
			as.push_back(std::move(account));
		}

		int count = std::stoi(_a);
		if (count >= keys.size())
		{
			count = keys.size() - 1;
		}
		else if (count <= 0)
		{
			count = 1;
		}

		for (int i = 0; i < count; i++)
		{
			TransactionSkeleton ts;
			ts.type = TransactionType::MessageCall;
			//ts.from = Address(as[0].address);
			ts.to = Address(as[i + 1].address);
			ts.value = u256(1000000000000000);
			//ts.data = bytes();
			//ts.nonce = u256();
			ts.gas = u256(50000);
			ts.gasPrice = client()->gasBidPrice();

			Secret secret(as[0].secret);

			client()->submitTransaction(ts, secret);
		}

		return toJS(count);
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_testSend2(string const& _a)
{
	try
	{
		struct myAccount {
			string address;
			string secret;
		};
		std::vector<myAccount> as;
		string filePath(boost::filesystem::current_path().string());
		string s = contentsString(filePath + "/address-keys.json");
		json_spirit::mValue v;
		json_spirit::read_string(s, v);
		json_spirit::mObject keys = v.get_obj();
		for (const auto& key : keys)
		{
			myAccount account{ key.first , key.second.get_str() };
			as.push_back(std::move(account));
		}

		int count = std::stoi(_a);
		if (count >= keys.size())
		{
			count = keys.size() - 1;
		}
		else if (count <= 0)
		{
			count = 1;
		}

		for (int i = 0; i < count; i++)
		{
			TransactionSkeleton ts;
			ts.type = TransactionType::MessageCall;
			//ts.from = Address(as[0].address);
			ts.to = Address(as[0].address);
			ts.value = u256(1000000000000000);
			//ts.data = bytes();
			//ts.nonce = u256();
			ts.gas = u256(50000);
			ts.gasPrice = client()->gasBidPrice();

			Secret secret(as[i + 1].secret);

			client()->submitTransaction(ts, secret);
		}

		return toJS(count);
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_testSendBlock(string const& _a)
{
	try
	{
		Block block(ChainParams().accountStartNonce);
		//BlockChain bc();
		//block.sync(bc);

		TransactionQueue tq;

		block.setAuthor(Address());

		int count = std::stoi(_a);
		return toJS(count);
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_sign(string const& _a, string const& _b)
{
	try
	{
		if (_b.size() != 64)
		{
			return "Message length error!";
		}
		struct myAccount {
			string address;
			string secret;
		};
		string filePath(boost::filesystem::current_path().string());
		string s = contentsString(filePath + "/name-keys.json");
		json_spirit::mValue v;
		json_spirit::read_string(s, v);
		json_spirit::mObject name_keys = v.get_obj();
		if (!name_keys.count(_a))
		{
			return "Not find keys!";
		}
		myAccount ma;
		for (auto i : name_keys[_a].get_obj())
		{
			ma.address = i.first;
			ma.secret = i.second.get_str();
		}

		Secret secret(ma.secret);
		bytes b_bytes = fromHex(_b);
		h256 h;
		memcpy(h.data(), b_bytes.data(), 32);
		Signature sig = sign(secret, h);

		return sig.hex();
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}
