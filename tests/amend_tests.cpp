#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <fc/variant_object.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <iostream>
#include <boost/container/map.hpp>
#include <map>

#include <Runtime/Runtime.h>
#include <iomanip>

#include "amend_tester.hpp"

using namespace eosio;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
using namespace std;
using namespace decidetesting::testing;
using namespace testing;
using mvo = fc::mutable_variant_object;

BOOST_AUTO_TEST_SUITE(amend_tests)

    BOOST_FIXTURE_TEST_CASE( configuration_setting, amend_tester ) try {

        //======================== check initial values ========================

        //initialize
        string app_name = "Telos Amend";
        string app_version = "v2.0.0";
        name initial_admin = name("amend.decide");
        asset initial_deposits = asset::from_string("0.0000 TLOS");
        double quorum_thresh = 5.0;
        double yes_thresh = 66.67;

        //get config table
        fc::variant amend_config = get_amend_config();
        map<name, asset> config_fee_map = variant_to_map<name, asset>(amend_config["fees"]);

        //assert table values
        BOOST_REQUIRE_EQUAL(amend_config["app_name"], app_name);
        BOOST_REQUIRE_EQUAL(amend_config["app_version"], app_version);
        BOOST_REQUIRE_EQUAL(amend_config["admin"].as<name>(), initial_admin);
        BOOST_REQUIRE_EQUAL(amend_config["deposits"].as<asset>(), initial_deposits);
        validate_map(config_fee_map, name("newdoc"), asset::from_string("50.0000 TLOS"));
        validate_map(config_fee_map, name("newproposal"), asset::from_string("10.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(amend_config["quorum_threshold"].as<double>(), quorum_thresh);
        BOOST_REQUIRE_EQUAL(amend_config["yes_threshold"].as<double>(), yes_thresh);

        //======================== change version ========================

        //initialize
        string new_version = "v2.0.1";

        //send setversion trx
        amend_setversion(new_version, initial_admin);
        produce_blocks();

        //get new config table
        amend_config = get_amend_config();

        //check app version updated
        BOOST_REQUIRE_EQUAL(amend_config["app_version"], new_version);

        //======================== change admin ========================

        //initialize
        name new_amend_admin = name("testaccountb");

        //send setadmin trx
        amend_setadmin(new_amend_admin, initial_admin);
        produce_blocks();

        //get new config table
        amend_config = get_amend_config();

        //check admin updated
        BOOST_REQUIRE_EQUAL(amend_config["admin"].as<name>(), new_amend_admin);

        //======================== set new thresholds ========================

        //initialize
        double new_quorum_thresh = 5.5;
        double new_yes_thresh = 85.0;

        //send setadmin trx
        amend_setthresh(new_quorum_thresh, new_yes_thresh, new_amend_admin);
        produce_blocks();

        //get new config table
        amend_config = get_amend_config();

        //check admin updated
        BOOST_REQUIRE_EQUAL(amend_config["quorum_threshold"].as<double>(), new_quorum_thresh);
        BOOST_REQUIRE_EQUAL(amend_config["yes_threshold"].as<double>(), new_yes_thresh);

    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( deposit_withdraw, amend_tester ) try {

        //======================== deposit funds ========================

        //send transfers to amend
        base_tester::transfer(testa, amend_name, "100.0000 TLOS", "", token_name);
        base_tester::transfer(testb, amend_name, "50.0000 TLOS", "", token_name);
        base_tester::transfer(testc, amend_name, "1.0000 TLOS", "", token_name);
        produce_blocks();

        //get tables
        fc::variant testa_amend_acct = get_amend_account(testa, tlos_sym);
        fc::variant testb_amend_acct = get_amend_account(testb, tlos_sym);
        fc::variant testc_amend_acct = get_amend_account(testc, tlos_sym);
        fc::variant amend_conf = get_amend_config();

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_amend_acct["balance"].as<asset>(), asset::from_string("100.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(testb_amend_acct["balance"].as<asset>(), asset::from_string("50.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(testc_amend_acct["balance"].as<asset>(), asset::from_string("1.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(amend_conf["deposits"].as<asset>(), asset::from_string("151.0000 TLOS"));

        //======================== withdraw ========================

        //withdraw half of tlos back to eosio.token balance
        amend_withdraw(testa, asset::from_string("50.0000 TLOS"));
        amend_withdraw(testb, asset::from_string("25.0000 TLOS"));
        produce_blocks();

        //get udpated tables
        testa_amend_acct = get_amend_account(testa, tlos_sym);
        testb_amend_acct = get_amend_account(testb, tlos_sym);
        amend_conf = get_amend_config();

        //assert new balance
        BOOST_REQUIRE_EQUAL(testa_amend_acct["balance"].as<asset>(), asset::from_string("50.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(testb_amend_acct["balance"].as<asset>(), asset::from_string("25.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(amend_conf["deposits"].as<asset>(), asset::from_string("76.0000 TLOS"));

    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( document_creation_editing, amend_tester ) try {

        //======================== deposit funds ========================

        //fund account balance
        base_tester::transfer(testa, amend_name, "100.0000 TLOS", "", token_name);
        produce_blocks();

        //get tables
        fc::variant testa_amend_acct = get_amend_account(testa, tlos_sym);

        //assert account balance
        BOOST_REQUIRE_EQUAL(testa_amend_acct["balance"].as<asset>(), asset::from_string("100.0000 TLOS"));

        //======================== create new document ========================

        //create test document
        create_test_document();
        produce_blocks();

        //======================== edit header ========================

        //initialize
        string new_title = "Updated Document 1 Title";
        string new_subtitle = "Updated Document 1 Subtitle";

        //send editheader trx
        amend_editheader(doc1_name, new_title, new_subtitle, testa);
        produce_blocks();

        //get tables
        fc::variant amend_doc1 = get_amend_document(doc1_name);

        //assert table values
        BOOST_REQUIRE_EQUAL(amend_doc1["title"], new_title);
        BOOST_REQUIRE_EQUAL(amend_doc1["subtitle"], new_subtitle);

        //======================== udpate author ========================

        //initialize
        name new_author = testb;

        //send updateauthor trx
        amend_updateauthor(doc1_name, new_author, testa);
        produce_blocks();

        //get tables
        amend_doc1 = get_amend_document(doc1_name);

        //assert table values
        BOOST_REQUIRE_EQUAL(amend_doc1["author"].as<name>(), new_author);

        //======================== reorder sections ========================

        //initialize
        map<name, uint64_t> new_order;
        new_order[name("sectiona")] = 3;
        new_order[name("sectionb")] = 2;
        new_order[name("sectionc")] = 1;

        //send updateauthor trx
        amend_reorder(doc1_name, new_order, new_author);
        produce_blocks();

        //get tables
        fc::variant amend_sectiona = get_amend_section(doc1_name, name("sectiona"));
        fc::variant amend_sectionb = get_amend_section(doc1_name, name("sectionb"));
        fc::variant amend_sectionc = get_amend_section(doc1_name, name("sectionc"));

        //assert table values
        BOOST_REQUIRE_EQUAL(amend_sectiona["section_number"].as<uint64_t>(), 3);
        BOOST_REQUIRE_EQUAL(amend_sectionb["section_number"].as<uint64_t>(), 2);
        BOOST_REQUIRE_EQUAL(amend_sectionc["section_number"].as<uint64_t>(), 1);


    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( proposal_draft, amend_tester ) try {

        //======================== deposit funds ========================

        //fund account balance
        base_tester::transfer(testa, amend_name, "100.0000 TLOS", "", token_name);
        produce_blocks();

        //get tables
        fc::variant testa_amend_acct = get_amend_account(testa, tlos_sym);

        //assert account balance
        BOOST_REQUIRE_EQUAL(testa_amend_acct["balance"].as<asset>(), asset::from_string("100.0000 TLOS"));

        //======================== create new document ========================

        //create test document
        create_test_document();
        produce_blocks();

        //======================== draft proposal ========================

        //initialize
        string title = "Proposal 1";
        string subtitle = "Telos Amend Proposal 1";
        name prop_name = name("amendprop1");
        map<name, string> new_content;
        string new_section_c_text = "Updated Section C Text";
        string section_d_text = "Section D Text";
        new_content[name("sectionc")] = new_section_c_text;
        new_content[name("sectiond")] = section_d_text;

        //push draftprop trx
        amend_draftprop(title, subtitle, prop_name, testa, doc1_name, new_content);
        produce_blocks();

        //get proposal
        fc::variant amend_prop = get_amend_proposal(prop_name);
        map<name, asset> amend_results_map = variant_to_map<name, asset>(amend_prop["ballot_results"]);
        map<name, string> amend_new_content_map = variant_to_map<name, string>(amend_prop["new_content"]);

        //assert table values
        BOOST_REQUIRE_EQUAL(amend_prop["title"], title);
        BOOST_REQUIRE_EQUAL(amend_prop["subtitle"], subtitle);
        BOOST_REQUIRE_EQUAL(amend_prop["ballot_name"].as<name>(), prop_name);
        BOOST_REQUIRE_EQUAL(amend_prop["document_name"].as<name>(), doc1_name);
        BOOST_REQUIRE_EQUAL(amend_prop["status"].as<name>(), name("drafting"));
        BOOST_REQUIRE_EQUAL(amend_prop["treasury_symbol"].as<symbol>(), vote_sym);
        BOOST_REQUIRE_EQUAL(amend_prop["proposer"].as<name>(), testa);

        validate_map(amend_results_map, name("yes"), asset::from_string("0.0000 VOTE"));
        validate_map(amend_results_map, name("no"), asset::from_string("0.0000 VOTE"));
        validate_map(amend_results_map, name("abstain"), asset::from_string("0.0000 VOTE"));

        validate_map(amend_new_content_map, name("sectionc"), new_section_c_text);
        validate_map(amend_new_content_map, name("sectiond"), section_d_text);

    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( proposal_complete, amend_tester ) try {
        
        //======================== deposit funds ========================

        //fund account balance
        base_tester::transfer(testa, amend_name, "100.0000 TLOS", "", token_name);
        produce_blocks();

        //get tables
        fc::variant testa_amend_acct = get_amend_account(testa, tlos_sym);

        //assert account balance
        BOOST_REQUIRE_EQUAL(testa_amend_acct["balance"].as<asset>(), asset::from_string("100.0000 TLOS"));

        //======================== create new document ========================

        //create test document
        create_test_document();
        produce_blocks();

        //======================== draft proposal ========================

        //initialize
        string title = "Proposal 1";
        string subtitle = "Telos Amend Proposal 1";
        name prop_name = name("amendprop1");
        map<name, string> new_content;
        string new_section_c_text = "Updated Section C Text";
        string section_d_text = "Section D Text";
        new_content[name("sectionc")] = new_section_c_text;
        new_content[name("sectiond")] = section_d_text;

        //push draftprop trx
        amend_draftprop(title, subtitle, prop_name, testa, doc1_name, new_content);
        produce_blocks();

        //get proposal
        fc::variant amend_prop = get_amend_proposal(prop_name);
        map<name, asset> amend_results_map = variant_to_map<name, asset>(amend_prop["ballot_results"]);
        map<name, string> amend_new_content_map = variant_to_map<name, string>(amend_prop["new_content"]);

        //assert table values
        BOOST_REQUIRE_EQUAL(amend_prop["title"], title);
        BOOST_REQUIRE_EQUAL(amend_prop["subtitle"], subtitle);
        BOOST_REQUIRE_EQUAL(amend_prop["ballot_name"].as<name>(), prop_name);
        BOOST_REQUIRE_EQUAL(amend_prop["document_name"].as<name>(), doc1_name);
        BOOST_REQUIRE_EQUAL(amend_prop["status"].as<name>(), name("drafting"));
        BOOST_REQUIRE_EQUAL(amend_prop["treasury_symbol"].as<symbol>(), vote_sym);
        BOOST_REQUIRE_EQUAL(amend_prop["proposer"].as<name>(), testa);

        validate_map(amend_results_map, name("yes"), asset::from_string("0.0000 VOTE"));
        validate_map(amend_results_map, name("no"), asset::from_string("0.0000 VOTE"));
        validate_map(amend_results_map, name("abstain"), asset::from_string("0.0000 VOTE"));

        validate_map(amend_new_content_map, name("sectionc"), new_section_c_text);
        validate_map(amend_new_content_map, name("sectiond"), section_d_text);

        //======================== launch proposal ========================

        //get proposal, config, and proposer account
        amend_prop = get_amend_proposal(prop_name);
        fc::variant amend_conf = get_amend_config();
        testa_amend_acct = get_amend_account(testa, tlos_sym);

        //initialize
        asset old_balance = testa_amend_acct["balance"].as<asset>();
        asset decide_ballot_fee = asset::from_string("10.0000 TLOS"); //NOTE: proposal fee covers decide fee

        //calculate
        asset new_proposer_balance = testa_amend_acct["balance"].as<asset>() - decide_ballot_fee;

        //launch proposal trx
        amend_launchprop(prop_name, testa);
        produce_blocks();

        //get new tables
        fc::variant amend_doc1 = get_amend_document(doc1_name);
        amend_prop = get_amend_proposal(prop_name);
        testa_amend_acct = get_amend_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(amend_doc1["open_proposals"].as<uint16_t>(), 1);

        BOOST_REQUIRE_EQUAL(amend_prop["status"].as<name>(), name("voting"));

        BOOST_REQUIRE_EQUAL(testa_amend_acct["balance"].as<asset>(), new_proposer_balance);

        //======================== cast votes ========================

        //initialize
        vector<name> yes_vote = { name("yes") };

        //cast votes
        cast_vote(testa, prop_name, yes_vote);
        cast_vote(testb, prop_name, yes_vote);
        cast_vote(testc, prop_name, yes_vote);
        produce_blocks();

        //======================== end proposal ========================

        //advance time
        produce_blocks();
        produce_block(fc::seconds(1'000'000));
        produce_blocks();

        //send endprop trx
        amend_endprop(prop_name, testa);
        produce_blocks();

        //get tables
        amend_prop = get_amend_proposal(prop_name);

        //assert table values
        BOOST_REQUIRE_EQUAL(amend_prop["status"].as<name>(), name("passed"));

        //======================== amend document ========================

        //send amendprop action
        amend_amendprop(prop_name, testb);
        produce_blocks();

        //get tables
        amend_doc1 = get_amend_document(doc1_name);
        amend_prop = get_amend_proposal(prop_name);
        amend_results_map = variant_to_map<name, asset>(amend_prop["ballot_results"]);
        fc::variant amend_doc1_sectiona = get_amend_section(doc1_name, name("sectiona"));
        fc::variant amend_doc1_sectionb = get_amend_section(doc1_name, name("sectionb"));
        fc::variant amend_doc1_sectionc = get_amend_section(doc1_name, name("sectionc"));
        fc::variant amend_doc1_sectiond = get_amend_section(doc1_name, name("sectiond"));

        //assert table values
        BOOST_REQUIRE_EQUAL(amend_doc1["sections"].as<uint16_t>(), 4);
        BOOST_REQUIRE_EQUAL(amend_doc1["open_proposals"].as<uint16_t>(), 0);
        BOOST_REQUIRE_EQUAL(amend_doc1["amendments"].as<uint16_t>(), 1);

        BOOST_REQUIRE_EQUAL(amend_prop["status"].as<name>(), name("amended"));
        validate_map(amend_results_map, name("yes"), asset::from_string("560.0000 VOTE"));
        validate_map(amend_results_map, name("no"), asset::from_string("0.0000 VOTE"));
        validate_map(amend_results_map, name("abstain"), asset::from_string("0.0000 VOTE"));
        
        // BOOST_REQUIRE_EQUAL(amend_doc1_sectiona["section_number"].as<uint64_t>(), 0);

        // BOOST_REQUIRE_EQUAL(amend_doc1_sectionb["section_number"].as<uint64_t>(), 1);

        BOOST_REQUIRE_EQUAL(amend_doc1_sectionc["section_number"].as<uint64_t>(), 2);
        BOOST_REQUIRE_EQUAL(amend_doc1_sectionc["content"], new_section_c_text);
        // BOOST_REQUIRE_EQUAL(amend_doc1_sectionc["last_amended"].as<time_point_sec>(), now);
        BOOST_REQUIRE_EQUAL(amend_doc1_sectionc["amended_by"].as<name>(), testb);

        BOOST_REQUIRE_EQUAL(amend_doc1_sectiond["section_name"].as<name>(), name("sectiond"));
        BOOST_REQUIRE_EQUAL(amend_doc1_sectiond["section_number"].as<uint64_t>(), 0);
        BOOST_REQUIRE_EQUAL(amend_doc1_sectiond["content"], section_d_text);
        // BOOST_REQUIRE_EQUAL(amend_doc1_sectiond["last_amended"].as<time_point_sec>(), now);
        BOOST_REQUIRE_EQUAL(amend_doc1_sectiond["amended_by"].as<name>(), testb);

        //======================== delete proposal ========================

        //clean up votes to allow decide ballot deletion
        cleanup_vote(testa, testa, prop_name, { testa });
        cleanup_vote(testa, testb, prop_name, { testa });
        cleanup_vote(testa, testc, prop_name, { testa });

        //advance time
        produce_blocks();
        produce_block(fc::days(5));
        produce_blocks();

        //send deleteprop trx
        amend_deleteprop(prop_name, testa);
        produce_blocks();

    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( proposal_fail, amend_tester ) try {

        //======================== deposit funds ========================

        //fund account balance
        base_tester::transfer(testa, amend_name, "100.0000 TLOS", "", token_name);
        produce_blocks();

        //get tables
        fc::variant testa_amend_acct = get_amend_account(testa, tlos_sym);

        //assert account balance
        BOOST_REQUIRE_EQUAL(testa_amend_acct["balance"].as<asset>(), asset::from_string("100.0000 TLOS"));

        //======================== create new document ========================

        //create test document
        create_test_document();
        produce_blocks();

        //======================== draft proposal ========================

        //initialize
        string title = "Proposal 1";
        string subtitle = "Telos Amend Proposal 1";
        name prop_name = name("amendprop1");
        map<name, string> new_content;
        string new_section_c_text = "Updated Section C Text";
        string section_d_text = "Section D Text";
        new_content[name("sectionc")] = new_section_c_text;
        new_content[name("sectiond")] = section_d_text;

        //push draftprop trx
        amend_draftprop(title, subtitle, prop_name, testa, doc1_name, new_content);
        produce_blocks();

        //get proposal
        fc::variant amend_prop = get_amend_proposal(prop_name);
        map<name, asset> amend_results_map = variant_to_map<name, asset>(amend_prop["ballot_results"]);
        map<name, string> amend_new_content_map = variant_to_map<name, string>(amend_prop["new_content"]);

        //assert table values
        BOOST_REQUIRE_EQUAL(amend_prop["title"], title);
        BOOST_REQUIRE_EQUAL(amend_prop["subtitle"], subtitle);
        BOOST_REQUIRE_EQUAL(amend_prop["ballot_name"].as<name>(), prop_name);
        BOOST_REQUIRE_EQUAL(amend_prop["document_name"].as<name>(), doc1_name);
        BOOST_REQUIRE_EQUAL(amend_prop["status"].as<name>(), name("drafting"));
        BOOST_REQUIRE_EQUAL(amend_prop["treasury_symbol"].as<symbol>(), vote_sym);
        BOOST_REQUIRE_EQUAL(amend_prop["proposer"].as<name>(), testa);

        validate_map(amend_results_map, name("yes"), asset::from_string("0.0000 VOTE"));
        validate_map(amend_results_map, name("no"), asset::from_string("0.0000 VOTE"));
        validate_map(amend_results_map, name("abstain"), asset::from_string("0.0000 VOTE"));

        validate_map(amend_new_content_map, name("sectionc"), new_section_c_text);
        validate_map(amend_new_content_map, name("sectiond"), section_d_text);

        //======================== launch proposal ========================

        //get proposal, config, and proposer account
        amend_prop = get_amend_proposal(prop_name);
        fc::variant amend_conf = get_amend_config();
        testa_amend_acct = get_amend_account(testa, tlos_sym);

        //initialize
        asset old_balance = testa_amend_acct["balance"].as<asset>();
        asset decide_ballot_fee = asset::from_string("10.0000 TLOS"); //NOTE: proposal fee covers decide fee

        //calculate
        asset new_proposer_balance = testa_amend_acct["balance"].as<asset>() - decide_ballot_fee;

        //launch proposal trx
        amend_launchprop(prop_name, testa);
        produce_blocks();

        //get new tables
        fc::variant amend_doc1 = get_amend_document(doc1_name);
        amend_prop = get_amend_proposal(prop_name);
        testa_amend_acct = get_amend_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(amend_doc1["open_proposals"].as<uint16_t>(), 1);

        BOOST_REQUIRE_EQUAL(amend_prop["status"].as<name>(), name("voting"));

        BOOST_REQUIRE_EQUAL(testa_amend_acct["balance"].as<asset>(), new_proposer_balance);

        //======================== cast votes ========================

        //initialize
        vector<name> yes_vote = { name("yes") };
        vector<name> no_vote = { name("no") };
        vector<name> abs_vote = { name("abstain") };

        //cast votes
        cast_vote(testa, prop_name, yes_vote);
        cast_vote(testb, prop_name, no_vote);
        cast_vote(testc, prop_name, abs_vote);
        produce_blocks();

        //======================== end proposal ========================

        //advance time
        produce_blocks();
        produce_block(fc::seconds(1'000'000));
        produce_blocks();

        //send endprop trx
        amend_endprop(prop_name, testa);
        produce_blocks();

        //get tables
        amend_prop = get_amend_proposal(prop_name);

        //assert table values
        BOOST_REQUIRE_EQUAL(amend_prop["status"].as<name>(), name("failed"));

        //attempt to amend failed proposal
        BOOST_REQUIRE_EXCEPTION(amend_amendprop(prop_name, testb),
            eosio_assert_message_exception, eosio_assert_message_is( "proposal must have passed to amend document" ) 
        );

        //======================== delete proposal ========================

        //clean up votes to allow decide ballot deletion
        cleanup_vote(testa, testa, prop_name, { testa });
        cleanup_vote(testa, testb, prop_name, { testa });
        cleanup_vote(testa, testc, prop_name, { testa });

        //advance time
        produce_blocks();
        produce_block(fc::days(5));
        produce_blocks();

        //send deleteprop trx
        amend_deleteprop(prop_name, testa);
        produce_blocks();

    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( proposal_cancel, amend_tester ) try {
        
        //======================== deposit funds ========================

        //fund account balance
        base_tester::transfer(testa, amend_name, "100.0000 TLOS", "", token_name);
        produce_blocks();

        //get tables
        fc::variant testa_amend_acct = get_amend_account(testa, tlos_sym);

        //assert account balance
        BOOST_REQUIRE_EQUAL(testa_amend_acct["balance"].as<asset>(), asset::from_string("100.0000 TLOS"));

        //======================== create new document ========================

        //create test document
        create_test_document();
        produce_blocks();

        //======================== draft proposal ========================

        //initialize
        string title = "Proposal 1";
        string subtitle = "Telos Amend Proposal 1";
        name prop_name = name("amendprop1");
        map<name, string> new_content;
        string new_section_c_text = "Updated Section C Text";
        string section_d_text = "Section D Text";
        new_content[name("sectionc")] = new_section_c_text;
        new_content[name("sectiond")] = section_d_text;

        //push draftprop trx
        amend_draftprop(title, subtitle, prop_name, testa, doc1_name, new_content);
        produce_blocks();

        //get proposal
        fc::variant amend_prop = get_amend_proposal(prop_name);
        map<name, asset> amend_results_map = variant_to_map<name, asset>(amend_prop["ballot_results"]);
        map<name, string> amend_new_content_map = variant_to_map<name, string>(amend_prop["new_content"]);

        //assert table values
        BOOST_REQUIRE_EQUAL(amend_prop["title"], title);
        BOOST_REQUIRE_EQUAL(amend_prop["subtitle"], subtitle);
        BOOST_REQUIRE_EQUAL(amend_prop["ballot_name"].as<name>(), prop_name);
        BOOST_REQUIRE_EQUAL(amend_prop["document_name"].as<name>(), doc1_name);
        BOOST_REQUIRE_EQUAL(amend_prop["status"].as<name>(), name("drafting"));
        BOOST_REQUIRE_EQUAL(amend_prop["treasury_symbol"].as<symbol>(), vote_sym);
        BOOST_REQUIRE_EQUAL(amend_prop["proposer"].as<name>(), testa);

        validate_map(amend_results_map, name("yes"), asset::from_string("0.0000 VOTE"));
        validate_map(amend_results_map, name("no"), asset::from_string("0.0000 VOTE"));
        validate_map(amend_results_map, name("abstain"), asset::from_string("0.0000 VOTE"));

        validate_map(amend_new_content_map, name("sectionc"), new_section_c_text);
        validate_map(amend_new_content_map, name("sectiond"), section_d_text);

        //======================== launch proposal ========================

        //get proposal, config, and proposer account
        amend_prop = get_amend_proposal(prop_name);
        fc::variant amend_conf = get_amend_config();
        testa_amend_acct = get_amend_account(testa, tlos_sym);

        //initialize
        asset old_balance = testa_amend_acct["balance"].as<asset>();
        asset decide_ballot_fee = asset::from_string("10.0000 TLOS"); //NOTE: proposal fee covers decide fee

        //calculate
        asset new_proposer_balance = testa_amend_acct["balance"].as<asset>() - decide_ballot_fee;

        //launch proposal trx
        amend_launchprop(prop_name, testa);
        produce_blocks();

        //get new tables
        fc::variant amend_doc1 = get_amend_document(doc1_name);
        amend_prop = get_amend_proposal(prop_name);
        testa_amend_acct = get_amend_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(amend_doc1["open_proposals"].as<uint16_t>(), 1);

        BOOST_REQUIRE_EQUAL(amend_prop["status"].as<name>(), name("voting"));

        BOOST_REQUIRE_EQUAL(testa_amend_acct["balance"].as<asset>(), new_proposer_balance);

        //======================== cancel proposal ========================

        //send cancelprop trx
        amend_cancelprop(prop_name, string("Works Proposal 1 Cancellation"), testa);
        produce_blocks();

        //get tables
        amend_prop = get_amend_proposal(prop_name);

        //assert table values
        BOOST_REQUIRE_EQUAL(amend_prop["status"].as<name>(), name("cancelled"));

        //attempt to amend cancelled proposal
        BOOST_REQUIRE_EXCEPTION(amend_amendprop(prop_name, testb),
            eosio_assert_message_exception, eosio_assert_message_is( "proposal must have passed to amend document" ) 
        );

        //======================== delete proposal ========================

        //advance time
        produce_blocks();
        produce_block(fc::days(36));
        produce_blocks();

        //send deleteprop trx
        amend_deleteprop(prop_name, testa);
        produce_blocks();
        
    } FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
