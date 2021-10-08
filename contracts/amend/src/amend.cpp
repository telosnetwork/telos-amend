#include "../include/amend.hpp"

//======================== admin actions ========================

ACTION amend::init(string app_name, string app_version, name initial_admin) {
    
    //authenticate
    require_auth(get_self());
    
    //open config singleton
    config_singleton configs(get_self(), get_self().value);
    
    //validate
    check(!configs.exists(), "config already exists");
    check(is_account(initial_admin), "initial admin account doesn't exist");
    
    //initialize
    asset initial_deposits = asset(0, TLOS_SYM);
    map<name, asset> initial_fees;
    initial_fees["newdoc"_n] = asset(500000, TLOS_SYM); //50 TLOS
    initial_fees["newproposal"_n] = asset(100000, TLOS_SYM); //10 TLOS
    double quorum_threshold = 5.0;
    double yes_threshold = 66.67;

    config initial_conf = {
        app_name, //app_name
        app_version, //app_version
        initial_admin, //admin
        initial_deposits, //deposits
        initial_fees, //fees
        quorum_threshold, //quorum_threshold
        yes_threshold //yes_threshold
    };

    //set new config
    configs.set(initial_conf, get_self());

}

ACTION amend::setversion(string new_version) {

    //open configs singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //change app version
    conf.app_version = new_version;

    //set new config
    configs.set(conf, get_self());

}

ACTION amend::setadmin(name new_admin) {

    //open configs singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //change admin
    conf.admin = new_admin;

    //set new config
    configs.set(conf, get_self());

}

ACTION amend::setfee(name fee_name, asset fee_amount) {

    //open configs singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //validate
    check(fee_amount.amount > 0, "fee must be a positive number");
    check(fee_amount.symbol == TLOS_SYM, "fee must be denominated in TLOS");

    //upsert fee
    conf.fees[fee_name] = fee_amount;

    //set new config
    configs.set(conf, get_self());

}

ACTION amend::setthresh(double new_quorum_threshold, double new_yes_threshold) {

    //open configs singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //change thresholds
    conf.quorum_threshold = new_quorum_threshold;
    conf.yes_threshold = new_yes_threshold;

    //set new config
    configs.set(conf, get_self());

}

//======================== document actions ========================

ACTION amend::newdocument(string title, string subtitle, name document_name, name author, map<name, string> initial_sections) {

    //open configs singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //open documents table, find document
    documents_table documents(get_self(), get_self().value);
    auto doc = documents.find(document_name.value);

    //authenticate
    require_auth(author);

    //validate
    check(doc == documents.end(), "document name already exists");

    //charge fee
    require_fee(author, conf.fees.at("newdoc"_n));

    //emplace new document
    documents.emplace(author, [&](auto& col) {
        col.title = title;
        col.subtitle = subtitle;
        col.document_name = document_name;
        col.author = author;
        col.sections = initial_sections.size();
        col.open_proposals = 0;
        col.amendments = 0;
    });

    //initialize
    uint64_t sec_order = 0;
    time_point_sec now = time_point_sec(current_time_point());

    //emplace each initial section
    for (auto itr = initial_sections.begin(); itr != initial_sections.end(); itr++) {

        //open sections table, find section
        sections_table sections(get_self(), document_name.value);
        auto sec_itr = sections.find(itr->first.value);

        //validate
        check(sec_itr == sections.end(), "section already exists");

        //emplace new section
        sections.emplace(author, [&](auto& col) {
            col.section_name = itr->first;
            col.section_number = sec_order;
            col.content = itr->second;
            col.last_amended = now;
            col.amended_by = author;
            
        //iterate
        sec_order += 1;
        });

    }

}

ACTION amend::editheader(name document_name, string new_title, string new_subtitle) {

    //open documents table, get document
    documents_table documents(get_self(), get_self().value);
    auto& doc = documents.get(document_name.value, "document not found");

    //authenticate
    require_auth(doc.author);

    //modify document title and subtitle
    documents.modify(doc, same_payer, [&](auto& col) {
        col.title = new_title;
        col.subtitle = new_subtitle;
    });
    
}

ACTION amend::updateauthor(name document_name, name new_author) {

    //open documents table, get document
    documents_table documents(get_self(), get_self().value);
    auto& doc = documents.get(document_name.value, "document not found");

    //authenticate
    require_auth(doc.author);

    //validate
    check(is_account(new_author), "new author account doesn't exist");

    //modify document author
    documents.modify(doc, same_payer, [&](auto& col) {
        col.author = new_author;
    });

}

ACTION amend::deldocument(name document_name, string memo) {

    //open documents table, get document
    documents_table documents(get_self(), get_self().value);
    auto& doc = documents.get(document_name.value, "document not found");

    //authenticate
    require_auth(doc.author);

    //validate
    check(doc.sections == 0, "document must have all sections deleted");

    //erase document
    documents.erase(doc);

}

//======================== section actions ========================

ACTION amend::reorder(name document_name, map<name, uint64_t> new_order) {

    //open documents table, get document
    documents_table documents(get_self(), get_self().value);
    auto& doc = documents.get(document_name.value, "document not found");

    //authenticate
    require_auth(doc.author);

    for (auto itr = new_order.begin(); itr != new_order.end(); itr++) {

        //open sections table, find section
        sections_table sections(get_self(), document_name.value);
        auto& sec = sections.get(itr->first.value, "section not found");

        //modify section
        sections.modify(sec, same_payer, [&](auto& col) {
            col.section_number = itr->second;
        });

    }

}

ACTION amend::delsection(name document_name, name section_name, string memo) {

    //open documents table, get document
    documents_table documents(get_self(), get_self().value);
    auto& doc = documents.get(document_name.value, "document not found");

    //authenticate
    require_auth(doc.author);

    //open sections table, find section
    sections_table sections(get_self(), document_name.value);
    auto& sec = sections.get(section_name.value, "section not found");

    //validate
    check(doc.sections > 0, "document is already empty");
    check(sec.content == "", "content must be cleared to erase");

    //erase section
    sections.erase(sec);

    //update document sections counter
    documents.modify(doc, same_payer, [&](auto& col) {
        col.sections -= 1;
    });

}

//======================== proposal actions ========================

ACTION amend::draftprop(string title, string subtitle, name ballot_name, name proposer, name document_name, map<name, string> new_content) {

    //open proposals table, find proposal
    proposals_table proposals(get_self(), get_self().value);
    auto prop_itr = proposals.find(ballot_name.value);

    //open documents table, get document
    documents_table documents(get_self(), get_self().value);
    auto& doc = documents.get(document_name.value, "document not found");

    //authenticate
    require_auth(proposer);

    //validate
    check(prop_itr == proposals.end(), "proposal ballot already exists");

    //initialize
    map<name, asset> blank_results;
    blank_results["yes"_n] = asset(0, VOTE_SYM);
    blank_results["no"_n] = asset(0, VOTE_SYM);
    blank_results["abstain"_n] = asset(0, VOTE_SYM);
    
    //emplace new proposal
    proposals.emplace(proposer, [&](auto& col) {
        col.title = title;
        col.subtitle = subtitle;
        col.ballot_name = ballot_name;
        col.document_name = document_name;
        col.status = name("drafting");
        col.treasury_symbol = VOTE_SYM;
        col.proposer = proposer;
        col.ballot_results = blank_results;
        col.new_content = new_content;
    });

}

ACTION amend::launchprop(name ballot_name) {

    //open proposals table, get proposal
    proposals_table proposals(get_self(), get_self().value);
    auto& prop = proposals.get(ballot_name.value, "proposal not found");

    //open documents table, get document
    documents_table documents(get_self(), get_self().value);
    auto& doc = documents.get(prop.document_name.value, "document not found");

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(prop.proposer);

    //initialize
    vector<name> initial_options = { "yes"_n, "no"_n, "abstain"_n };
    string ballot_content = "Telos Amend Proposal";
    time_point_sec now = time_point_sec(current_time_point());
    uint32_t month_sec = 2505600; //2505600 = 29 days in seconds
    asset proposal_fee = conf.fees.at("newproposal"_n);

    //validate
    check(prop.status == "drafting"_n, "proposal must be in drafting mode to launch");

    //update proposal status
    proposals.modify(prop, same_payer, [&](auto& col) {
        col.status = name("voting");
    });

    //update document open proposals counter
    documents.modify(doc, same_payer, [&](auto& col) {
        col.open_proposals += 1;
    });

    //charge fee
    require_fee(prop.proposer, proposal_fee);

    //inline transfer from self to telos decide (to cover decide newballot fee, paid for by amend proposal fee)
    action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"), make_tuple(
        get_self(), //from
        name("telos.decide"), //to
        proposal_fee, //quantity
        string("Telos Decide Ballot Fee") //memo
    )).send();

    //inline to newballot
    action(permission_level{get_self(), name("active")}, name("telos.decide"), name("newballot"), make_tuple(
        ballot_name, //ballot_name
        name("proposal"), //category
        get_self(), //publisher
        VOTE_SYM, //treasury_symbol
        name("1token1vote"), //voting_method
        initial_options //initial_options
    )).send();

    //inline to editdetails
    action(permission_level{get_self(), name("active")}, name("telos.decide"), name("editdetails"), make_tuple(
        ballot_name, //ballot_name
        prop.title, //title
        prop.subtitle, //description
        ballot_content //content
    )).send();

    //send inline togglebal
    action(permission_level{get_self(), name("active")}, name("telos.decide"), name("togglebal"), make_tuple(
        ballot_name, //ballot_name
        name("votestake") //setting_name
    )).send();

    //send inline openvoting
    action(permission_level{get_self(), name("active")}, name("telos.decide"), name("openvoting"), make_tuple(
        ballot_name, //ballot_name
        now + month_sec //end_time
    )).send();

}

ACTION amend::endprop(name ballot_name) {

    //open proposals table, get proposal
    proposals_table proposals(get_self(), get_self().value);
    auto& prop = proposals.get(ballot_name.value, "proposal not found");

    //open documents table, get document
    documents_table documents(get_self(), get_self().value);
    auto& doc = documents.get(prop.document_name.value, "document not found");

    //authenticate
    require_auth(prop.proposer);

    //validate
    check(prop.status == "voting"_n, "proposal must be voting to end");

    //update document open proposals counter
    documents.modify(doc, same_payer, [&](auto& col) {
        col.open_proposals -= 1;
    });

    //NOTE: ballot results processing happens in catch_broadcast()

    //send inline closevoting
    action(permission_level{get_self(), name("active")}, name("telos.decide"), name("closevoting"), make_tuple(
        ballot_name, //ballot_name
        true //broadcast
    )).send();

}

ACTION amend::amendprop(name ballot_name, name amender) {

    //open proposals table, get proposals
    proposals_table proposals(get_self(), get_self().value);
    auto& prop = proposals.get(ballot_name.value, "proposal not found");

    //open documents table, get document
    documents_table documents(get_self(), get_self().value);
    auto& doc = documents.get(prop.document_name.value, "document not found");

    //authenticate
    require_auth(amender); //TODO?: executable only by author? only by proposer?

    //validate
    check(prop.status == name("passed"), "proposal must have passed to amend document");

    //intialize
    auto now = time_point_sec(current_time_point());
    uint16_t new_section_count = 0;

    for (auto itr = prop.new_content.begin(); itr != prop.new_content.end(); itr++) {

        //open sections table, find section
        sections_table sections(get_self(), prop.document_name.value);
        auto sec_itr = sections.find(itr->first.value);

        if (sec_itr == sections.end()) {

            //TODO: calculate actual section number

            //increase section count
            new_section_count += 1;

            //emplace new section, ram paid by amender
            sections.emplace(amender, [&](auto& col) {
                col.section_name = itr->first;
                col.section_number = 0;
                col.content = itr->second;
                col.last_amended = now;
                col.amended_by = amender;
            });

        } else {

            //update existing section, ram paid by amender
            sections.modify(*sec_itr, amender, [&](auto& col) {
                col.content = itr->second;
                col.last_amended = now;
                col.amended_by = amender;
            });

        }

    }

    //update document amendments counter
    documents.modify(doc, same_payer, [&](auto& col) {
        col.sections += new_section_count;
        col.amendments += 1;
    });

    //update proposal status to amended
    proposals.modify(prop, same_payer, [&](auto& col) {
        col.status = name("amended");
    });

}

ACTION amend::cancelprop(name ballot_name, string memo) {

    //open proposals table, get proposal
    proposals_table proposals(get_self(), get_self().value);
    auto& prop = proposals.get(ballot_name.value, "proposal ballot not found");

    //open documents table, get document
    documents_table documents(get_self(), get_self().value);
    auto& doc = documents.get(prop.document_name.value, "document not found");

    //authenticate
    require_auth(prop.proposer);

    //validate
    check(prop.status == "voting"_n, "proposal must be voting to cancel");

    //update document open proposals counter
    documents.modify(doc, same_payer, [&](auto& col) {
        col.open_proposals -= 1;
    });

    //cancel proposal
    proposals.modify(prop, same_payer, [&](auto& col) {
        col.status = name("cancelled");
    });

    //inline to cancelballot
    action(permission_level{get_self(), name("active")}, name("telos.decide"), name("cancelballot"), make_tuple(
        ballot_name, //ballot_name
        memo //memo
    )).send();

}

ACTION amend::deleteprop(name ballot_name) {

    //open proposals table, get proposal
    proposals_table proposals(get_self(), get_self().value);
    auto& prop = proposals.get(ballot_name.value, "proposal ballot not found");

    //authenticate
    require_auth(prop.proposer);

    //validate
    check(prop.status != name("voting"), "cannot delete a proposal during voting. cancel first.");

    //erase proposal
    proposals.erase(prop);

    //inline to deleteballot
    action(permission_level{get_self(), name("active")}, name("telos.decide"), name("deleteballot"), make_tuple(
        ballot_name //ballot_name
    )).send();

}

//======================== account actions ========================

ACTION amend::withdraw(name account_name, asset quantity) {

    //authenticate
    require_auth(account_name);

    //open accounts table, get account
    accounts_table accounts(get_self(), account_name.value);
    auto& acct = accounts.get(quantity.symbol.code().raw(), "account not found");

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //validate
    check(quantity.symbol == TLOS_SYM, "must withdraw TLOS");
    check(quantity.amount > 0, "must withdraw positive amount");
    check(quantity.amount <= acct.balance.amount, "insufficient account funds");
    check(conf.deposits.amount >= quantity.amount, "insufficient contract funds");

    //update account balance
    accounts.modify(acct, same_payer, [&](auto& col) {
        col.balance -= quantity;
    });

    //update config deposits
    conf.deposits -= quantity;
    configs.set(conf, get_self());

    //inline transfer
    action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"), make_tuple(
        get_self(), //from
        account_name, //to
        quantity, //quantity
        std::string("Telos Amend Withdrawal") //memo
    )).send();

}

//======================== notification handlers ========================

void amend::catch_transfer(name from, name to, asset quantity, string memo) {

    //get initial receiver contract
    name rec = get_first_receiver();

    //validate
    if (rec == name("eosio.token") && from != get_self() && quantity.symbol == TLOS_SYM) {
        
        //parse memo
        //skips emplacement if memo is skip
        if (memo == std::string("skip")) { 
            return;
        }
        
        //open accounts table, search for account
        accounts_table accounts(get_self(), from.value);
        auto acct = accounts.find(TLOS_SYM.code().raw());

        //emplace account if not found, update if exists
        if (acct == accounts.end()) {

            //make new account
            accounts.emplace(get_self(), [&](auto& col) {
                col.balance = quantity;
            });

        } else {

            //update existing account
            accounts.modify(*acct, same_payer, [&](auto& col) {
                col.balance += quantity;
            });

        }

        //open config singleton, get config
        config_singleton configs(get_self(), get_self().value);
        auto conf = configs.get();

        //update config
        conf.deposits += quantity;
        configs.set(conf, get_self());

    }

}

void amend::catch_broadcast(name ballot_name, map<name, asset> final_results, uint32_t total_voters) {
        
    //open proposals table, get by ballot index, find prop
    proposals_table proposals(get_self(), get_self().value);
    auto prop_itr = proposals.find(ballot_name.value);

    if (prop_itr != proposals.end()) {

        //open config singleton, get config
        config_singleton configs(get_self(), get_self().value);
        auto conf = configs.get();

        //open telos decide treasury table, get treasury
        treasuries_table treasuries(name("telos.decide"), name("telos.decide").value);
        auto& trs = treasuries.get(VOTE_SYM.code().raw(), "treasury not found");

        //calculate
        asset total_votes = final_results["yes"_n] + final_results["no"_n] + final_results["abstain"_n];
        asset non_abstain_votes = final_results["yes"_n] + final_results["no"_n];
        asset quorum_thresh = trs.supply * conf.quorum_threshold / 100;
        asset pass_thresh = non_abstain_votes * conf.yes_threshold / 100;

        //initialize
        name new_status = name(0);

        //determine approval and refund
        if (total_votes >= quorum_thresh && final_results["yes"_n] >= pass_thresh) {

            //passed
            new_status = name("passed");

        } else {

            //failed
            new_status = name("failed");

        }

        //update proposal
        proposals.modify(*prop_itr, same_payer, [&](auto& col) {
            col.ballot_results = final_results;
            col.status = new_status;
        });

    }

}

//======================== functions ========================

void amend::require_fee(name account_name, asset fee) {

    //open accounts table, get account
    accounts_table accounts(get_self(), account_name.value);
    auto& acct = accounts.get(TLOS_SYM.code().raw(), "require_fee: account not found");

    //open config singleton
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //validate
    check(acct.balance >= fee, "insufficient account funds");
    check(conf.deposits >= fee, "insufficient contract funds");

    //charge fee
    accounts.modify(acct, same_payer, [&](auto& col) {
        col.balance -= fee;
    });

    //update deposits
    conf.deposits -= fee;
    configs.set(conf, get_self());

}
