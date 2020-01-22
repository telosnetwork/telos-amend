#include <decide_tester.hpp>

#include <contracts.hpp>

using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;
using namespace decidetesting::testing;

using mvo = fc::mutable_variant_object;

class amend_tester : public decide_tester {
    
    public:
    
    abi_serializer amend_abi_ser;

    const name amend_name = name("amend.decide");
    const name doc1_name = name("document1");

    amend_tester(setup_mode mode = setup_mode::full): decide_tester(mode) {
        
        //initialize
        asset max_vote_supply = asset::from_string("1000000000.0000 VOTE");
        asset ram_amount = asset::from_string("400.0000 TLOS");
        asset liquid_amount = asset::from_string("10000.0000 TLOS");

        //create accounts
        create_account_with_resources(amend_name, eosio_name, ram_amount, false);
        produce_blocks();
        
        //setcode, setabi, init
        setup_amend_contract();
        produce_blocks();

        //create VOTE treasury
        base_tester::transfer(eosio_name, decide_name, "3000.0000 TLOS", "", token_name);
        new_treasury(eosio_name, max_vote_supply, name("public"));
        produce_blocks();

        //register amend as voter on telos decide
        reg_voter(amend_name, max_vote_supply.get_symbol(), {});
        produce_blocks();

        //delegate bw for test accounts
        delegate_bw(testa, testa, asset::from_string("10.0000 TLOS"), asset::from_string("90.0000 TLOS"), false);
        delegate_bw(testb, testb, asset::from_string("10.0000 TLOS"), asset::from_string("40.0000 TLOS"), false);
        delegate_bw(testc, testc, asset::from_string("10.0000 TLOS"), asset::from_string("340.0000 TLOS"), false);
        produce_blocks();

        //register voters on telos decide
        reg_voter(testa, max_vote_supply.get_symbol(), {});
        reg_voter(testb, max_vote_supply.get_symbol(), {});
        reg_voter(testc, max_vote_supply.get_symbol(), {});
        produce_blocks();

        //get voter balances
        fc::variant testa_voter = get_voter(testa, vote_sym);
        fc::variant testb_voter = get_voter(testb, vote_sym);
        fc::variant testc_voter = get_voter(testc, vote_sym);

        //assert VOTE balances
        BOOST_REQUIRE_EQUAL(testa_voter["staked"].as<asset>(), asset::from_string("120.0000 VOTE"));
        BOOST_REQUIRE_EQUAL(testb_voter["staked"].as<asset>(), asset::from_string("70.0000 VOTE"));
        BOOST_REQUIRE_EQUAL(testc_voter["staked"].as<asset>(), asset::from_string("370.0000 VOTE"));

    }

    //======================== setup functions ========================

    void setup_amend_contract() {

        //setcode, setabi
        set_code( amend_name, contracts::amend_wasm());
        set_abi( amend_name, contracts::amend_abi().data() );
        {
            const auto& accnt = control->db().get<account_object,by_name>( amend_name );
            abi_def abi;
            BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
            amend_abi_ser.set_abi(abi, abi_serializer_max_time);
        }

        //push init action to amend
        amend_init("Telos Amend", "v2.0.0", amend_name);
        produce_blocks();

        //initialize new auth
        authority auth = authority( get_public_key(amend_name, "active") );
        auth.accounts.push_back( permission_level_weight{ {amend_name, config::eosio_code_name}, static_cast<weight_type>(auth.threshold) } );

        //set eosio.code
        set_authority(amend_name, name("active"), auth, name("owner"));
        produce_blocks();

    }

    void create_test_document() {

        map<name, string> sections;
        sections[name("section1")] = "Section 1 Text";
        sections[name("section2")] = "Section 2 Text";
        sections[name("section3")] = "Section 3 Text";

        //create new document
        amend_newdocument("Document 1", "Test Document 1", doc1_name, testa, sections);
        produce_blocks();

        //get tables
        fc::variant doc1 = get_amend_document(doc1_name);
        fc::variant testa_amend_acct = get_amend_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_amend_acct["balance"].as<asset>(), asset::from_string("100.0000 TLOS"));

    }

    //======================== amend actions ===========================

    transaction_trace_ptr amend_init(string app_name, string app_version, name initial_admin) {
        signed_transaction trx;
        vector<permission_level> permissions { { amend_name, name("active") } };
        trx.actions.emplace_back(get_action(amend_name, name("init"), permissions, 
            mvo()
                ("app_name", app_name)
                ("app_version", app_version)
                ("initial_admin", initial_admin)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(amend_name, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr amend_setversion(string new_version, name admin_name) {
        signed_transaction trx;
        vector<permission_level> permissions { { admin_name, name("active") } };
        trx.actions.emplace_back(get_action(amend_name, name("setversion"), permissions, 
            mvo()
                ("new_version", new_version)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(admin_name, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr amend_setadmin(name new_admin, name old_admin) {
        signed_transaction trx;
        vector<permission_level> permissions { { old_admin, name("active") } };
        trx.actions.emplace_back(get_action(amend_name, name("setadmin"), permissions, 
            mvo()
                ("new_admin", new_admin)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(old_admin, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr amend_setfee(name fee_name, asset fee_amount, name admin_name) {
        signed_transaction trx;
        vector<permission_level> permissions { { admin_name, name("active") } };
        trx.actions.emplace_back(get_action(amend_name, name("setfee"), permissions, 
            mvo()
                ("fee_name", fee_name)
                ("fee_amount", fee_amount)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(admin_name, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr amend_setthresh(double new_quorum_threshold, double new_yes_threshold, name admin_name) {
        signed_transaction trx;
        vector<permission_level> permissions { { admin_name, name("active") } };
        trx.actions.emplace_back(get_action(amend_name, name("setthresh"), permissions, 
            mvo()
                ("new_quorum_threshold", new_quorum_threshold)
                ("new_yes_threshold", new_yes_threshold)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(admin_name, "active"), control->get_chain_id());
        return push_transaction( trx );
    }



    transaction_trace_ptr amend_newdocument(string title, string subtitle, name doc_name, name author, map<name, string> initial_sections) {
        signed_transaction trx;
        vector<permission_level> permissions { { author, name("active") } };
        trx.actions.emplace_back(get_action(amend_name, name("newdocument"), permissions, 
            mvo()
                ("title", title)
                ("subtitle", subtitle)
                ("document_name", doc_name)
                ("author", author)
                ("initial_sections", initial_sections)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(author, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr amend_editheader(name doc_name, string new_title, string new_subtitle, name author) {
        signed_transaction trx;
        vector<permission_level> permissions { { author, name("active") } };
        trx.actions.emplace_back(get_action(amend_name, name("editheader"), permissions, 
            mvo()
                ("document_name", doc_name)
                ("new_title", new_title)
                ("new_subtitle", new_subtitle)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(author, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr amend_updateauthor(name doc_name, name new_author, name old_author) {
        signed_transaction trx;
        vector<permission_level> permissions { { old_author, name("active") } };
        trx.actions.emplace_back(get_action(amend_name, name("updateauthor"), permissions, 
            mvo()
                ("document_name", doc_name)
                ("new_author", new_author)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(old_author, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr amend_deldocument(name doc_name, string memo, name author) {
        signed_transaction trx;
        vector<permission_level> permissions { { author, name("active") } };
        trx.actions.emplace_back(get_action(amend_name, name("deldocument"), permissions, 
            mvo()
                ("document_name", doc_name)
                ("memo", memo)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(author, "active"), control->get_chain_id());
        return push_transaction( trx );
    }



    transaction_trace_ptr amend_draftprop(string title, string subtitle, name ballot_name, 
        name proposer, name document_name, map<name, string> new_content) {
        signed_transaction trx;
        vector<permission_level> permissions { { proposer, name("active") } };
        trx.actions.emplace_back(get_action(amend_name, name("draftprop"), permissions, 
            mvo()
                ("title", title)
                ("subtitle", subtitle)
                ("ballot_name", ballot_name)
                ("proposer", proposer)
                ("document_name", document_name)
                ("new_content", new_content)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(proposer, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr amend_launchprop(name proposal_name, name proposer) {
        signed_transaction trx;
        vector<permission_level> permissions { { proposer, name("active") } };
        trx.actions.emplace_back(get_action(amend_name, name("launchprop"), permissions, 
            mvo()
                ("proposal_name", proposal_name)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(proposer, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr amend_endprop(name proposal_name, name proposer) {
        signed_transaction trx;
        vector<permission_level> permissions { { proposer, name("active") } };
        trx.actions.emplace_back(get_action(amend_name, name("endprop"), permissions, 
            mvo()
                ("proposal_name", proposal_name)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(proposer, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr amend_cancelprop(name proposal_name, string memo, name proposer) {
        signed_transaction trx;
        vector<permission_level> permissions { { proposer, name("active") } };
        trx.actions.emplace_back(get_action(amend_name, name("cancelprop"), permissions, 
            mvo()
                ("proposal_name", proposal_name)
                ("memo", memo)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(proposer, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr amend_deleteprop(name proposal_name, name proposer) {
        signed_transaction trx;
        vector<permission_level> permissions { { proposer, name("active") } };
        trx.actions.emplace_back(get_action(amend_name, name("deleteprop"), permissions, 
            mvo()
                ("proposal_name", proposal_name)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(proposer, "active"), control->get_chain_id());
        return push_transaction( trx );
    }



    transaction_trace_ptr amend_withdraw(name account_name, asset quantity) {
        signed_transaction trx;
        vector<permission_level> permissions { { account_name, name("active") } };
        trx.actions.emplace_back(get_action(amend_name, name("withdraw"), permissions, 
            mvo()
                ("account_name", account_name)
                ("quantity", quantity)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(account_name, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    //======================== amend getters ========================

    fc::variant get_amend_config() { 
        vector<char> data = get_row_by_account(amend_name, amend_name, name("config"), name("config"));
        return data.empty() ? fc::variant() : amend_abi_ser.binary_to_variant("config", data, abi_serializer_max_time);
    }

    fc::variant get_amend_document(name document_name) { 
        vector<char> data = get_row_by_account(amend_name, amend_name, name("documents"), document_name);
        return data.empty() ? fc::variant() : amend_abi_ser.binary_to_variant("document", data, abi_serializer_max_time);
    }

    fc::variant get_amend_section(name document_name, name section_name) { 
        vector<char> data = get_row_by_account(amend_name, document_name, name("sections"), section_name);
        return data.empty() ? fc::variant() : amend_abi_ser.binary_to_variant("section", data, abi_serializer_max_time);
    }

    fc::variant get_amend_proposal(name ballot_name) { 
        vector<char> data = get_row_by_account(amend_name, amend_name, name("proposals"), ballot_name);
        return data.empty() ? fc::variant() : amend_abi_ser.binary_to_variant("proposal", data, abi_serializer_max_time);
    }

    fc::variant get_amend_account(name account_owner, symbol token_sym) { 
        vector<char> data = get_row_by_account(amend_name, account_owner, name("accounts"), token_sym.to_symbol_code());
        return data.empty() ? fc::variant() : amend_abi_ser.binary_to_variant("account", data, abi_serializer_max_time);
    }

};