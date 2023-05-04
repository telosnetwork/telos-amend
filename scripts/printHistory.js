import {APIClient} from '@greymass/eosio'
import axios from 'axios'

const url = 'https://mainnet.telos.net'
const hyperionUrl = 'https://mainnet.telos.net'
const sectionScope = 'tbnoa'

;(async () => {
    const client = new APIClient({url})
    const history = axios.create({
        baseURL: hyperionUrl
    })

    const deltas = await history.get(`v2/history/get_deltas?code=amend.decide&scope=${sectionScope}&table=sections&limit=5000`)
    if (deltas.status !== 200)
        throw new Error('Non 200 from hyperion')

    console.log(`Total count:     ${deltas.data.total.value}`)
    console.log(`Deltas received: ${deltas.data.deltas.length}`)
    if (deltas.data.total.value !== deltas.data.deltas.length)
        throw new Error("Unable to get all deltas in one request with limit set to 5000")

    const amendments = {}
    for (let row of deltas.data.deltas.reverse()) {
        const {section_name: name, section_number: number, content, last_amended: lastAmended, amended_by: amendedBy} = row.data
        let amendment

        // Group by last amended value, assuming that each amendment has all it's updates in a unique timestamp/second
        if (!amendments.hasOwnProperty(lastAmended)) {
            amendment = {
                lastAmended, amendedBy, sections: {}
            }
            amendments[lastAmended] = amendment
        } else {
            amendment = amendments[lastAmended]
        }

        if (amendment.sections.hasOwnProperty(number))
            throw new Error("Section modified twice in the same amendment (actually in the same second, but we assume each second will only have one amendment)")

        amendment.sections[number] = { name, number: parseInt(number), content }
    }

    const orderedAmendments = Object.values(amendments).sort((a, b) => {
        return Date.parse(a.lastAmended) - Date.parse(b.lastAmended)
    })

    orderedAmendments.forEach(amendment => {
        amendment.sections = Object.values(amendment.sections).sort((a, b) => {
            return a.number - b.number
        })
    })

    for (let amendment of orderedAmendments) {
        console.log(`======================`)
        console.log(`On ${amendment.lastAmended} ${amendment.amendedBy} modified ${amendment.sections.length} sections: `)
        for (let section of amendment.sections) {
            console.log(`${section.number} ${section.name} - ${section.content}`)
            try {
                const contentData = await getContent(section.content)
                console.log(contentData)
            } catch (e) {
                console.log(`ERROR fetching content for hash: ${section.content}`)
            }
        }
        console.log();
        console.log();
    }

})()


async function getContent(contentHash) {
    const dstorResponse = await axios.get(`https://api.dstor.cloud/ipfs/${contentHash}`)
    return dstorResponse.data
}