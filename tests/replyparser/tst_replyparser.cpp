#include <QtTest>
#include <QObject>
#include <QMap>
#include <QString>

#include "replyparser_p.h"
#include "syncer_p.h"
#include "carddav_p.h"

#include <QContact>
#include <QContactDisplayLabel>
#include <QContactName>
#include <QContactPhoneNumber>
#include <QContactGuid>
#include <qtcontacts-extensions.h>

QTCONTACTS_USE_NAMESPACE

typedef QMap<QString, QString> QMapStringString;
typedef QMap<QString, ReplyParser::FullContactInformation> QMapStringFullContactInfo;

namespace {

void dumpContactDetail(const QContactDetail &d)
{
    qWarning() << "++ ---------" << d.type();
    QMap<int, QVariant> values = d.values();
    foreach (int key, values.keys()) {
        qWarning() << "    " << key << "=" << values.value(key);
    }
}

void dumpContact(const QContact &c)
{
    qWarning() << "++++ ---- Contact:" << c.id();
    QList<QContactDetail> cdets = c.details();
    foreach (const QContactDetail &det, cdets) {
        dumpContactDetail(det);
    }
}

QContact removeIgnorableFields(const QContact &c)
{
    QContact ret;
    ret.setId(c.id());
    QList<QContactDetail> cdets = c.details();
    foreach (const QContactDetail &det, cdets) {
        QContactDetail d = det;
        d.removeValue(QContactDetail__FieldProvenance);
        d.removeValue(QContactDetail__FieldModifiable);
        d.removeValue(QContactDetail__FieldNonexportable);
        ret.saveDetail(&d);
    }
    return ret;
}

}

class tst_replyparser : public QObject
{
    Q_OBJECT

public:
    tst_replyparser()
        : m_s(Q_NULLPTR, Q_NULLPTR)
        , m_rp(&m_s, &m_vcc) {}

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void parseUserPrincipal_data();
    void parseUserPrincipal();

    void parseAddressbookHome_data();
    void parseAddressbookHome();

    void parseAddressbookInformation_data();
    void parseAddressbookInformation();

    void parseSyncTokenDelta_data();
    void parseSyncTokenDelta();

    void parseContactMetadata_data();
    void parseContactMetadata();

    void parseContactData_data();
    void parseContactData();

private:
    CardDavVCardConverter m_vcc;
    Syncer m_s;
    ReplyParser m_rp;
};

void tst_replyparser::initTestCase()
{
}

void tst_replyparser::cleanupTestCase()
{
}

void tst_replyparser::parseUserPrincipal_data()
{
    QTest::addColumn<QString>("xmlFilename");
    QTest::addColumn<QString>("expectedUserPrincipal");
    QTest::addColumn<int>("expectedResponseType");

    QTest::newRow("empty user information response")
        << QStringLiteral("data/replyparser_userprincipal_empty.xml")
        << QString()
        << static_cast<int>(ReplyParser::UserPrincipalResponse);

    QTest::newRow("single user principal in well-formed response")
        << QStringLiteral("data/replyparser_userprincipal_single-well-formed.xml")
        << QStringLiteral("/principals/users/johndoe/")
        << static_cast<int>(ReplyParser::UserPrincipalResponse);
}

void tst_replyparser::parseUserPrincipal()
{
    QFETCH(QString, xmlFilename);
    QFETCH(QString, expectedUserPrincipal);
    QFETCH(int, expectedResponseType);

    QFile f(QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(), xmlFilename));
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        QFAIL("Data file does not exist or cannot be opened for reading!");
    }

    QByteArray userInformationResponse = f.readAll();
    ReplyParser::ResponseType responseType = ReplyParser::UserPrincipalResponse;
    QString userPrincipal = m_rp.parseUserPrincipal(userInformationResponse, &responseType);

    QCOMPARE(userPrincipal, expectedUserPrincipal);
    QCOMPARE(responseType, static_cast<ReplyParser::ResponseType>(expectedResponseType));
}

void tst_replyparser::parseAddressbookHome_data()
{
    QTest::addColumn<QString>("xmlFilename");
    QTest::addColumn<QString>("expectedAddressbooksHomePath");

    QTest::newRow("empty addressbook urls response")
        << QStringLiteral("data/replyparser_addressbookhome_empty.xml")
        << QString();

    QTest::newRow("single well-formed addressbook urls set response")
        << QStringLiteral("data/replyparser_addressbookhome_single-well-formed.xml")
        << QStringLiteral("/addressbooks/johndoe/");
}

void tst_replyparser::parseAddressbookHome()
{
    QFETCH(QString, xmlFilename);
    QFETCH(QString, expectedAddressbooksHomePath);

    QFile f(QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(), xmlFilename));
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        QFAIL("Data file does not exist or cannot be opened for reading!");
    }

    QByteArray addressbookHomeSetResponse = f.readAll();
    QString addressbooksHomePath = m_rp.parseAddressbookHome(addressbookHomeSetResponse);

    QCOMPARE(addressbooksHomePath, expectedAddressbooksHomePath);
}

void tst_replyparser::parseAddressbookInformation_data()
{
    QTest::addColumn<QString>("xmlFilename");
    QTest::addColumn<QString>("addressbooksHomePath");
    QTest::addColumn<QList<ReplyParser::AddressBookInformation> >("expectedAddressbookInformation");

    QTest::newRow("empty addressbook information response")
        << QStringLiteral("data/replyparser_addressbookinformation_empty.xml")
        << QString()
        << QList<ReplyParser::AddressBookInformation>();

    QList<ReplyParser::AddressBookInformation> infos;
    ReplyParser::AddressBookInformation a;
    a.url = QStringLiteral("/addressbooks/johndoe/contacts/");
    a.displayName = QStringLiteral("My Address Book");
    a.ctag = QStringLiteral("3145");
    a.syncToken = QStringLiteral("http://sabredav.org/ns/sync-token/3145");
    infos << a;
    QTest::newRow("single addressbook information in well-formed response")
        << QStringLiteral("data/replyparser_addressbookinformation_single-well-formed.xml")
        << QStringLiteral("/addressbooks/johndoe/")
        << infos;

    infos.clear();
    ReplyParser::AddressBookInformation a2;
    a2.url = QStringLiteral("/addressbooks/johndoe/contacts/");
    a2.displayName = QStringLiteral("Contacts");
    a2.ctag = QStringLiteral("12345");
    a2.syncToken = QString();
    infos << a2;
    QTest::newRow("addressbook information in response including non-collection resources")
        << QStringLiteral("data/replyparser_addressbookinformation_addressbook-plus-contact.xml")
        << QStringLiteral("/addressbooks/johndoe/")
        << infos;

    infos.clear();
    ReplyParser::AddressBookInformation a3;
    a3.url = QStringLiteral("/dav/johndoe/contacts.vcf/");
    a3.displayName = QStringLiteral("Contacts");
    a3.ctag = QStringLiteral("22222");
    a3.syncToken = QString();
    infos << a3;
    QTest::newRow("addressbook information in response including principal and calendar collection")
        << QStringLiteral("data/replyparser_addressbookinformation_addressbook-calendar-principal.xml")
        << QStringLiteral("/dav/johndoe/")
        << infos;
}

bool operator==(const ReplyParser::AddressBookInformation& first, const ReplyParser::AddressBookInformation& second)
{
    return first.url == second.url
        && first.displayName == second.displayName
        && first.ctag == second.ctag
        && first.syncToken == second.syncToken;
}

void tst_replyparser::parseAddressbookInformation()
{
    QFETCH(QString, xmlFilename);
    QFETCH(QString, addressbooksHomePath);
    QFETCH(QList<ReplyParser::AddressBookInformation>, expectedAddressbookInformation);

    QFile f(QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(), xmlFilename));
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        QFAIL("Data file does not exist or cannot be opened for reading!");
    }

    QByteArray addressbookInformationResponse = f.readAll();
    QList<ReplyParser::AddressBookInformation> addressbookInfo = m_rp.parseAddressbookInformation(addressbookInformationResponse, addressbooksHomePath);

    QCOMPARE(addressbookInfo, expectedAddressbookInformation);
}

void tst_replyparser::parseSyncTokenDelta_data()
{
    QTest::addColumn<QString>("xmlFilename");
    QTest::addColumn<QMapStringString>("injectContactUris");
    QTest::addColumn<QString>("expectedNewSyncToken");
    QTest::addColumn<QList<ReplyParser::ContactInformation> >("expectedContactInformation");

    QList<ReplyParser::ContactInformation> infos;
    QTest::newRow("empty sync token delta response")
        << QStringLiteral("data/replyparser_synctokendelta_empty.xml")
        << QMap<QString, QString>()
        << QString()
        << infos;

    infos.clear();
    ReplyParser::ContactInformation c1;
    c1.modType = ReplyParser::ContactInformation::Addition;
    c1.uri = QStringLiteral("/addressbooks/johndoe/contacts/newcard.vcf");
    c1.guid = QString();
    c1.etag = QStringLiteral("\"33441-34321\"");
    infos << c1;
    QTest::newRow("single contact addition in well-formed sync token delta response")
        << QStringLiteral("data/replyparser_synctokendelta_single-well-formed-addition.xml")
        << QMap<QString, QString>()
        << QString()
        << infos;

    infos.clear();
    ReplyParser::ContactInformation c2;
    c2.modType = ReplyParser::ContactInformation::Modification;
    c2.uri = QStringLiteral("/addressbooks/johndoe/contacts/updatedcard.vcf");
    c2.guid = QStringLiteral("updatedcard_guid");
    c2.etag = QStringLiteral("\"33541-34696\"");
    ReplyParser::ContactInformation c3;
    c3.modType = ReplyParser::ContactInformation::Deletion;
    c3.uri = QStringLiteral("/addressbooks/johndoe/contacts/deletedcard.vcf");
    c3.guid = QStringLiteral("deletedcard_guid");
    c3.etag = QString();
    infos << c1 << c2 << c3;
    QMap<QString, QString> mContactUris;
    mContactUris.insert(c2.guid, c2.uri);
    mContactUris.insert(c3.guid, c3.uri);
    QTest::newRow("single contact addition + modification + removal in well-formed sync token delta response")
        << QStringLiteral("data/replyparser_synctokendelta_single-well-formed-add-mod-rem.xml")
        << mContactUris
        << QStringLiteral("http://sabredav.org/ns/sync/5001")
        << infos;
}

bool operator==(const ReplyParser::ContactInformation& first, const ReplyParser::ContactInformation& second)
{
    return first.modType == second.modType
        && first.uri == second.uri
        && first.guid == second.guid
        && first.etag == second.etag;
}

void tst_replyparser::parseSyncTokenDelta()
{
    QFETCH(QString, xmlFilename);
    QFETCH(QMapStringString, injectContactUris);
    QFETCH(QString, expectedNewSyncToken);
    QFETCH(QList<ReplyParser::ContactInformation>, expectedContactInformation);

    QFile f(QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(), xmlFilename));
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        QFAIL("Data file does not exist or cannot be opened for reading!");
    }

    m_s.m_contactUris = injectContactUris;

    QString newSyncToken;
    QByteArray syncTokenDeltaResponse = f.readAll();
    QList<ReplyParser::ContactInformation> contactInfo = m_rp.parseSyncTokenDelta(syncTokenDeltaResponse, &newSyncToken);

    QCOMPARE(newSyncToken, expectedNewSyncToken);
    QCOMPARE(contactInfo.size(), expectedContactInformation.size());
    if (contactInfo != expectedContactInformation) {
        for (int i = 0; i < contactInfo.size(); ++i) {
            if (!(contactInfo[i] == expectedContactInformation[i])) {
                qWarning() << "  actual:"
                           << contactInfo[i].modType
                           << contactInfo[i].uri
                           << contactInfo[i].guid
                           << contactInfo[i].etag;
                qWarning() << "expected:"
                           << expectedContactInformation[i].modType
                           << expectedContactInformation[i].uri
                           << expectedContactInformation[i].guid
                           << expectedContactInformation[i].etag;
            }
        }
        QFAIL("contact information different");
    }

    m_s.m_contactUris.clear();
}

void tst_replyparser::parseContactMetadata_data()
{
    QTest::addColumn<QString>("xmlFilename");
    QTest::addColumn<QString>("addressbookUrl");
    QTest::addColumn<QMapStringString>("injectContactUris");
    QTest::addColumn<QMapStringString>("injectContactEtags");
    QTest::addColumn<QList<ReplyParser::ContactInformation> >("expectedContactInformation");

    QList<ReplyParser::ContactInformation> infos;
    QTest::newRow("empty contact metadata response")
        << QStringLiteral("data/replyparser_contactmetadata_empty.xml")
        << QStringLiteral("/addressbooks/johndoe/contacts/")
        << QMap<QString, QString>()
        << QMap<QString, QString>()
        << infos;

    infos.clear();
    ReplyParser::ContactInformation c1;
    c1.modType = ReplyParser::ContactInformation::Addition;
    c1.uri = QStringLiteral("/addressbooks/johndoe/contacts/newcard.vcf");
    c1.guid = QString();
    c1.etag = QStringLiteral("\"0001-0001\"");
    ReplyParser::ContactInformation c2;
    c2.modType = ReplyParser::ContactInformation::Modification;
    c2.uri = QStringLiteral("/addressbooks/johndoe/contacts/updatedcard.vcf");
    c2.guid = QStringLiteral("updatedcard_guid");
    c2.etag = QStringLiteral("\"0002-0002\"");
    ReplyParser::ContactInformation c3;
    c3.modType = ReplyParser::ContactInformation::Deletion;
    c3.uri = QStringLiteral("/addressbooks/johndoe/contacts/deletedcard.vcf");
    c3.guid = QStringLiteral("deletedcard_guid");
    c3.etag = QStringLiteral("\"0003-0001\"");
    ReplyParser::ContactInformation c4;
    c4.modType = ReplyParser::ContactInformation::Uninitialized;
    c4.uri = QStringLiteral("/addressbooks/johndoe/contacts/unchangedcard.vcf");
    c4.guid = QStringLiteral("unchangedcard_guid");
    c4.etag = QStringLiteral("\"0004-0001\"");
    infos << c1 << c2 << c3; // but not c4, it's unchanged.
    QMap<QString, QString> mContactUris;
    mContactUris.insert(c2.guid, c2.uri);
    mContactUris.insert(c3.guid, c3.uri);
    mContactUris.insert(c4.guid, c4.uri);
    QMap<QString, QString> mContactEtags;
    mContactEtags.insert(c2.guid, QStringLiteral("\"0002-0001\"")); // changed to 0002-0002
    mContactEtags.insert(c3.guid, QStringLiteral("\"0003-0001\"")); // unchanged but deleted
    mContactEtags.insert(c4.guid, QStringLiteral("\"0004-0001\"")); // unchanged.
    QTest::newRow("single contact addition + modification + removal + unchanged in well-formed sync token delta response")
        << QStringLiteral("data/replyparser_contactmetadata_single-well-formed-add-mod-rem-unch.xml")
        << QStringLiteral("/addressbooks/johndoe/contacts/")
        << mContactUris
        << mContactEtags
        << infos;
}

void tst_replyparser::parseContactMetadata()
{
    QFETCH(QString, xmlFilename);
    QFETCH(QString, addressbookUrl);
    QFETCH(QMapStringString, injectContactUris);
    QFETCH(QMapStringString, injectContactEtags);
    QFETCH(QList<ReplyParser::ContactInformation>, expectedContactInformation);

    QFile f(QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(), xmlFilename));
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        QFAIL("Data file does not exist or cannot be opened for reading!");
    }

    m_s.m_contactUris = injectContactUris;
    m_s.m_contactEtags = injectContactEtags;
    m_s.m_addressbookContactGuids[addressbookUrl] = injectContactUris.keys();

    QByteArray contactMetadataResponse = f.readAll();
    QList<ReplyParser::ContactInformation> contactInfo = m_rp.parseContactMetadata(contactMetadataResponse, addressbookUrl);

    QCOMPARE(contactInfo, expectedContactInformation);

    m_s.m_addressbookContactGuids.clear();
    m_s.m_contactEtags.clear();
    m_s.m_contactUris.clear();
}

void tst_replyparser::parseContactData_data()
{
    QTest::addColumn<QString>("xmlFilename");
    QTest::addColumn<QString>("addressbookUrl");
    QTest::addColumn<QMapStringString>("injectContactUids");
    QTest::addColumn<QMapStringFullContactInfo>("expectedContactInformation");

    QMap<QString, ReplyParser::FullContactInformation> infos;
    QTest::newRow("empty contact data response")
        << QStringLiteral("data/replyparser_contactdata_empty.xml")
        << QStringLiteral("/addressbooks/johndoe/contacts/")
        << QMap<QString, QString>()
        << infos;

    infos.clear();
    QContact contact;
    QContactDisplayLabel cd;
    cd.setLabel(QStringLiteral("Testy Testperson"));
    QContactName cn;
    cn.setFirstName(QStringLiteral("Testy"));
    cn.setLastName(QStringLiteral("Testperson"));
    QContactPhoneNumber cp;
    cp.setNumber(QStringLiteral("555333111"));
    cp.setContexts(QList<int>() << QContactDetail::ContextHome);
    cp.setSubTypes(QList<int>() << QContactPhoneNumber::SubTypeMobile);
    QContactGuid cg;
    cg.setGuid(QStringLiteral("%1:AB:%2:%3").arg(QString::number(7357), QStringLiteral("/addressbooks/johndoe/contacts/"), QStringLiteral("testy-testperson-uid")));
    contact.saveDetail(&cd);
    contact.saveDetail(&cn);
    contact.saveDetail(&cp);
    contact.saveDetail(&cg);
    ReplyParser::FullContactInformation c1;
    c1.contact = contact;
    c1.unsupportedProperties = QStringList() << QStringLiteral("X-UNSUPPORTED-TEST-PROPERTY:7357");
    c1.etag = QStringLiteral("\"0001-0001\"");
    infos.insert(QStringLiteral("/addressbooks/johndoe/contacts/testytestperson.vcf"), c1);
    //QMap<QString, QString> mContactUids;
    //mContactUids.insert(cg.guid(), QStringLiteral("testy-testperson-uid"));
    QTest::newRow("single contact in well-formed contact data response")
        << QStringLiteral("data/replyparser_contactdata_single-well-formed.xml")
        << QStringLiteral("/addressbooks/johndoe/contacts/")
        << QMap<QString, QString>()
        << infos;
}

bool operator==(const ReplyParser::FullContactInformation& first, const ReplyParser::FullContactInformation& second)
{
    return first.unsupportedProperties == second.unsupportedProperties
        && first.etag == second.etag
        && first.contact == second.contact;
}

void tst_replyparser::parseContactData()
{
    QFETCH(QString, xmlFilename);
    QFETCH(QString, addressbookUrl);
    QFETCH(QMapStringString, injectContactUids);
    QFETCH(QMapStringFullContactInfo, expectedContactInformation);

    QFile f(QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(), xmlFilename));
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        QFAIL("Data file does not exist or cannot be opened for reading!");
    }

    m_s.m_accountId = 7357;
    m_s.m_contactUids = injectContactUids;

    QByteArray contactDataResponse = f.readAll();
    QMap<QString, ReplyParser::FullContactInformation> contactInfo = m_rp.parseContactData(contactDataResponse, addressbookUrl);

    QCOMPARE(contactInfo.size(), expectedContactInformation.size());
    QCOMPARE(contactInfo.keys(), expectedContactInformation.keys());
    Q_FOREACH (const QString &contactUri, contactInfo.keys()) {
        QCOMPARE(contactInfo[contactUri].unsupportedProperties, expectedContactInformation[contactUri].unsupportedProperties);
        QCOMPARE(contactInfo[contactUri].etag, expectedContactInformation[contactUri].etag);
        QContact actualContact = removeIgnorableFields(contactInfo[contactUri].contact);
        QContact expectedContact = removeIgnorableFields(expectedContactInformation[contactUri].contact);
        bool contactsAreDifferent = m_s.significantDifferences(&actualContact, &expectedContact);
        if (contactsAreDifferent) {
            qWarning() << "  actual:";
            dumpContact(actualContact);
            qWarning() << " expected:";
            dumpContact(expectedContact);
        }
        QVERIFY(!contactsAreDifferent);
    }

    m_s.m_contactUids.clear();
    m_s.m_accountId = 0;

    // parseContactData() can call migrateGuidData() so we clear it here.
    m_s.clearAllGuidData();
}

#include "tst_replyparser.moc"
QTEST_MAIN(tst_replyparser)
