# Starter Guide

## Contract Flow

1. Initialize

### ACTION init()
* app_name: "Telos Amend"
* app_version: "v0.1.0"
* initial_admin: "amendtester1"

2. Create New Document

### ACTION newdocument()
* title: "Document 1"
* subtitle: "A document for testing Telos Amend v0.1.0"
* document_name: "document1"
* author: "craig.tf"
* initial_sections: 

{
    "key": "section1",
    "value": "dStor//section1amend0"
},
{
    "key": "section2",
    "value": "dStor//section2amend0"
},
{
    "key": "section3",
    "value": "dStor//section3amend0"
},
{
    "key": "section4",
    "value": "dStor//section4amend0"
}

3. Draft Proposal

### ACTION draftprop()
* title: "Amendment 1"
* subtitle: "Document 1 Amendment 1"
* ballot_name: "doc1amend1"
* proposer: "craig.tf"
* document_name: "document1"
* new_content:

{
    "key": "section2",
    "value": "dstor://section2amend1"
},
{
    "key": "section5",
    "value": "dstor://section5amend0"
}

4. Launch Proposal

### ACTION launchprop()
* ballot_name: "doc1amend1"

5. Vote

### Vote through Telos Decide

6. Close Ballot

### ACTION endprop()
* ballot_name: "doc1amend1"

7. Amend Document

If proposal status = passed

### ACTION amend()
* document_name: "doc1amend1"
* new_sections: (from proposal)

{
    "key": "section2",
    "value": "dStor//section2amend1"
},
{
    "key": "section5",
    "value": "dStor//section5amend0"
}

8. Delete Proposal

You can delete the proposal from the table ro recover your ram. 

The proposal will always exist in block history.

### ACTION deleteprop()
* ballot_name: "doc1amend1"
