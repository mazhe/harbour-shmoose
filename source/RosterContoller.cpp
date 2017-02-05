#include "RosterContoller.h"

#include <QQmlContext>

RosterController::RosterController(QObject *parent) : QObject(parent), client_(NULL), rosterList_()
{
}

void RosterController::setClient(Swift::Client *client)
{
    client_ = client;
}

void RosterController::handleJidAdded(const Swift::JID &jid)
{
    std::cout << "RosterController::handleJidAdded: " << jid.toString() << std::endl;

    rosterList_.append(new RosterItem(QString::fromStdString(jid.toBare().toString()),
                                      "", // FIXME set name
                                      RosterItem::SUBSCRIPTION_NONE, this));
    emit rosterListChanged();
}

void RosterController::handleJidRemoved(const Swift::JID &jid)
{
    std::cout << "RosterController::handleJidRemoved: " << jid.toString() << std::endl;

    QList<RosterItem*>::iterator it = rosterList_.begin();
    while (it != rosterList_.end())
    {
        if ((*it)->getJid() == QString::fromStdString(jid.toBare().toString()))
        {
            it = rosterList_.erase(it);
        }
        else
        {
            ++it;
        }
    }

   emit rosterListChanged();
}

void RosterController::requestRosterFromClient(Swift::Client *client)
{
	client_->requestRoster();

    Swift::GetRosterRequest::ref rosterRequest = Swift::GetRosterRequest::create(client->getIQRouter());
    rosterRequest->onResponse.connect(bind(&RosterController::handleRosterReceived, this, _2));
    rosterRequest->send();
}

void RosterController::handleRosterReceived(Swift::ErrorPayload::ref error)
{
	if (error)
	{
		std::cerr << "Error receiving roster. Continuing anyway.";
	}
	else
	{
		//std::cout << "handleRosterReceived!!!" << std::endl;
		Swift::XMPPRoster* roster = client_->getRoster();
		std::vector<Swift::XMPPRosterItem> rosterItems = roster->getItems();

		std::vector<Swift::XMPPRosterItem>::iterator it;
		//std::cout << "size: " << rosterItems.size() << std::endl;

		for(it = rosterItems.begin(); it < rosterItems.end(); it++ )
		{
#if 0
			std::cout << "jid: " << (*it).getJID().toString() <<
						 ", Name: " << (*it).getName() <<
						 ", Subscription: " << (*it).getSubscription() << std::endl;
#endif

            rosterList_.append(new RosterItem(QString::fromStdString((*it).getJID().toBare().toString()),
                                              QString::fromStdString((*it).getName()),
                                              (RosterItem::Subscription)(*it).getSubscription(), this));

			emit rosterListChanged();
		}
	}

    // process updates in own methodes
    Swift::XMPPRoster *xmppRoster = client_->getRoster();
    xmppRoster->onJIDAdded.connect(boost::bind(&RosterController::handleJidAdded, this, _1));
    xmppRoster->onJIDRemoved.connect(boost::bind(&RosterController::handleJidRemoved, this, _1));
}

void RosterController::addContact(const QString& jid, const QString& name)
{
    Swift::IDGenerator idGenerator;
    std::string msgId = idGenerator.generateID();
    boost::shared_ptr<Swift::RosterPayload> payload(new Swift::RosterPayload);
    payload->addItem(Swift::RosterItemPayload(Swift::JID(jid.toStdString()), name.toStdString(), Swift::RosterItemPayload::None));
    //riPayload.setSubscriptionRequested();
    Swift::IQRouter *iqRouter = client_->getIQRouter();
    iqRouter->sendIQ(Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID(), msgId, payload));
}

void RosterController::removeContact(const QString& jid)
{
    Swift::IDGenerator idGenerator;
    std::string msgId = idGenerator.generateID();
    boost::shared_ptr<Swift::RosterPayload> payload(new Swift::RosterPayload);
    payload->addItem(Swift::RosterItemPayload(Swift::JID(jid.toStdString()), "", Swift::RosterItemPayload::Remove));
    Swift::IQRouter *iqRouter = client_->getIQRouter();
    iqRouter->sendIQ(Swift::IQ::createRequest(Swift::IQ::Set, Swift::JID(), msgId, payload));
}

QQmlListProperty<RosterItem> RosterController::getRosterList()
{
	return QQmlListProperty<RosterItem>(this, rosterList_);
}

