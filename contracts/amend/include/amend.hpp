// Telos Amend is a Decentralized Documentation Service for the Telos Blockchain Network.
//
// @author Craig Branscom
// @contract amend
// @version v0.1.0

// approved treasuries: VOTE

// proposal statuses: drafting, voting, passed, failed, cancelled, amended

#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/asset.hpp>

using namespace std;
using namespace eosio;

CONTRACT amend : public contract {

    public:

    amend(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {};

    ~amend() {};

    const symbol TLOS_SYM = symbol("TLOS", 4);
    const symbol VOTE_SYM = symbol("VOTE", 4);

    //======================== admin actions ========================

    //initialize contract
    ACTION init(string app_name, string app_version, name initial_admin);

    //set app version
    ACTION setversion(string new_version);

    //set admin
    ACTION setadmin(name new_admin);

    //set a fee amount
    ACTION setfee(name fee_name, asset fee_amount);

    //======================== document actions ========================

    //create a new document
    ACTION newdocument(string title, string subtitle, name document_name, name author, map<name, string> initial_sections);

    //edit document title and subtitle
    ACTION editdocinfo(name document_name, string new_title, string new_subtitle);

    //update document author
    ACTION updateauthor(name document_name, name new_author);

    //delete a document
    ACTION deldocument(name document_name, string memo);

    //======================== section actions ========================

    //reorder document section(s)
    ACTION reorder(name document_name, map<name, uint64_t> new_order);

    //delete a section
    ACTION delsection(name document_name, name section_name, string memo);

    //======================== proposal actions ========================

    //draft a new proposal
    ACTION draftprop(string title, string subtitle, name ballot_name, name proposer, name document_name, map<name, string> new_content);

    //launch a proposal
    ACTION launchprop(name ballot_name);

    //end a proposal after a complete vote
    ACTION endprop(name ballot_name);

    //update or add document section(s) from a proposal ballot
    ACTION amendprop(name ballot_name, name amender);

    //cancel a proposal
    ACTION cancelprop(name ballot_name, string memo);

    //delete a proposal
    ACTION deleteprop(name ballot_name);

    //======================== account actions ========================

    //withdraw to eosio.token account
    ACTION withdraw(name account_name, asset quantity);

    //======================== notification handlers ========================

    //catch transfer from eosio.token
    [[eosio::on_notify("eosio.token::transfer")]]
    void catch_transfer(name from, name to, asset quantity, string memo);

    //catch broadcast notification from trail
    [[eosio::on_notify("trailservice::broadcast")]]
    void catch_broadcast(name ballot_name, map<name, asset> final_results, uint32_t total_voters);

    //======================== functions ========================

    //require a charge on an account
    void require_fee(name account_name, asset fee);

    //======================== contract tables ========================

    //config singleton
    //scope: self
    TABLE config {
        string app_name;
        string app_version;
        name admin;
        map<name, asset> fees; // fee_name => fee_amount
        // vector<symbol> approved_treasuries;

        EOSLIB_SERIALIZE(config, (app_name)(app_version)(admin)(fees))
    };
    typedef singleton<name("config"), config> config_singleton;

    //document index
    //scope: self
    TABLE document {
        string title;
        string subtitle;
        name document_name;
        name author;
        uint16_t sections;
        uint16_t open_proposals;
        uint16_t amendments;
        // vector<symbol> approved_treasuries;

        uint64_t primary_key() const { return document_name.value; }
        uint64_t by_author() const { return author.value;  }
        EOSLIB_SERIALIZE(document, (title)(subtitle)(document_name)
            (author)(sections)(open_proposals)(amendments))
    };
    typedef multi_index<name("documents"), document,
        indexed_by<name("byauthor"), const_mem_fun<document, uint64_t, &document::by_author>>
    > documents_table;

    //section data
    //scope: document_name.value
    TABLE section {
        name section_name;
        uint64_t section_number;
        string content;
        time_point_sec last_amended;
        name amended_by;

        uint64_t primary_key() const { return section_name.value; }
        uint64_t by_number() const { return section_number; }
        EOSLIB_SERIALIZE(section, (section_name)(section_number)(content)(last_amended)(amended_by))
    };
    typedef multi_index<name("sections"), section,
        indexed_by<name("bynumber"), const_mem_fun<section, uint64_t, &section::by_number>>
    > sections_table;

    //amendment proposal
    //scope: self
    TABLE proposal {
        string title;
        string subtitle;
        name ballot_name; //name of Telos Decide ballot
        name document_name; //document to amend
        name status; //drafting, voting, passed, failed, cancelled, amended
        symbol treasury_symbol;
        name proposer;
        map<name, asset> ballot_results; //option_name => total_votes
        map<name, string> new_content; //section_name => content

        uint64_t primary_key() const { return ballot_name.value; }
        uint64_t by_document() const { return document_name.value; }
        uint64_t by_status() const { return status.value; }
        uint64_t by_symbol() const { return treasury_symbol.code().raw(); }
        EOSLIB_SERIALIZE(proposal, (title)(subtitle)(ballot_name)(document_name)
            (status)(treasury_symbol)(proposer)(ballot_results)(new_content))
    };
    typedef multi_index<name("proposals"), proposal,
        indexed_by<name("bydocument"), const_mem_fun<proposal, uint64_t, &proposal::by_document>>,
        indexed_by<name("bystatus"), const_mem_fun<proposal, uint64_t, &proposal::by_status>>,
        indexed_by<name("bysymbol"), const_mem_fun<proposal, uint64_t, &proposal::by_symbol>>
    > proposals_table;

    //account data
    //scope: account_owner.value
    TABLE account {
        asset balance;

        uint64_t primary_key() const { return balance.symbol.code().raw(); }
        EOSLIB_SERIALIZE(account, (balance))
    };
    typedef multi_index<name("accounts"), account> accounts_table;

};