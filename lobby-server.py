#!/usr/bin/env python
#!/Python27/python

""" -*- Mode: Python; tab-width: 4 -*-
    Author: Kristina Simpson (Krista^)
    
    Run as follows.
    Standalone mode: python lobby-server.py [<[-p|--port=] port number>] <game server url>
    Application server mode: twistd -y lobby-server.py [<[-p|--port=] port number>] <game server url>
"""
from twisted.internet import reactor
from twisted.web import server, resource, http
from twisted.web.server import Site, NOT_DONE_YET
from random import randint
from pprint import pprint
from optparse import OptionParser, OptionValueError
from time import strftime, time, localtime, sleep
from threading import Thread, RLock
import httplib, json
from urlparse import urlparse
import socket
from copy import deepcopy
from collections import deque
from random import choice
import logging

# Request timeout in seconds
REQUEST_TIMEOUT = 240
# Session expiry, in seconds
SESSION_TIMEOUT = REQUEST_TIMEOUT + 30
# Time after which we remove a session, in seconds.
SESSION_REMOVE_TIMEOUT = 3600

# Create a logger
logger = logging.getLogger('lobby-server')
logger.setLevel(logging.DEBUG)
ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
ch.setFormatter(logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'))
logger.addHandler(ch)

class QueueManager:
    def __init__(self):
        self._queue = {}
        self._queue_lock = RLock()
    def get_queue(self, name):
        with self._queue_lock:
            if not self._queue.has_key(name):
                logger.debug('Creating new queue for %s' % name)
                queue = {'clients':{'session_id':-1, 'name':name, 'ip_chain':None}, 'messages':{}, 'requests':[], 'timeouts':{}, 'lock':RLock()}
                self._queue[name] = queue
            return self._queue[name]
    def has_key(self, name):
        with self._queue_lock:
            return self._queue.has_key(name)
    def remove_queue(self, q):
        with self._queue_lock:
            name = q['clients']['name']
            logger.debug('Removing session for %s(%d)' % (name, q['clients']['session_id']))
            assert self._queue.has_key(name), 'Attempted delete of non-key %s' % name
            del self._queue[name]
        
queue_manager = QueueManager()

def make_session_id(): return randint(1, 2**31)
            
class GameServerStatusQueryThread(Thread):
    def __init__(self, game_server_url):
        Thread.__init__(self)
        self._last_seen = 0
        self._headers = {"Content-type":"application/json", "Accept":"text/json, application/json"}
        self._abort = False
        o = urlparse(game_server_url)
        self._game_server = o.netloc
        self._game_server_path = o.path
        self._stats_lock = RLock()
        self._game_stats = {'total_games_being_played': 0, 'played':{}}
    def run(self):
        running = True
        while running:
            if self._abort: running = False
            try:
                conn = httplib.HTTPConnection(self._game_server, timeout = 30)
                params = json.dumps({'type': "get_status", 'last_seen': self._last_seen})
                conn.request("POST", self._game_server_path, params, self._headers)
                response = conn.getresponse()
                #print response.status, response.reason
                if response.status == 200:
                    data = json.loads(response.read())
                    self._last_seen = data['status_id']
                    if data.has_key('games'):
                        self.parse_game_data(data['games'])
                    else:
                        print "Status packet received without 'games' data"
                conn.close()
                if not self._abort: sleep(10)
            except socket.timeout:
                print 'Timeout waiting for current status from game server'
            except Exception, e:
                print e
                sleep(10)
    def abort(self):
        self._abort = True
        self.join()
    def parse_game_data(self, games):
        """ Parses the game data returned from the server, data consists of a lists of items
            such as:
            {u'id': 1327454801, u'started': True, u'type': u'game_info'}
            type is always game_info (currently)
            id is always the id returned when the game was created.
            started is either True or False
        """
        # todo: go through the lobby_games_list and any games that aren't in the list from the game
        # server need to be removed and the references to them in the lobby_sessions need to be
        # removed.
        total_games_being_played = 0
        played = {}
    def get_game_stats(self):
        with self._stats_lock:
            game_stats = deepcopy(self._game_stats)
        return game_stats
    def ajax_post(self, msg):
        completion = False
        data = None
        conn = httplib.HTTPConnection(self._game_server, timeout = 30)
        params = json.dumps(msg)
        try:
            conn.request("POST", self._game_server_path, params, self._headers)
            response = conn.getresponse()
            #print response.status, response.reason
            if response.status == 200:
                data = json.loads(response.read())
                completion = True
            else:
                data = {'error':('%d: %s' % ( response.status, response.reason ))}
            conn.close()
        except socket.timeout:
            print 'Timeout waiting for message response from game server'
            return False, {'error': 'Timeout waiting for message response from game server'}
        except Exception, e:
            return False, {'error': str(e)}
        return completion, data

class LobbyHandler(resource.Resource):
    isLeaf = True
    def __init__(self, gsq):
        resource.Resource.__init__(self)
        self._gsq = gsq
        self._requests = []
        self._messages = {}
    def cleanup(self):
        pass
    def _delayedRender(self, q):
        with q['lock']:
            if q['timeouts'].has_key('delay'):
                del q['timeouts']['delay']
        request.write(json.dumps({'request':'success', 'type': 'status', 'status_id':0, messages:[]}))
        request.finish()
    def _responseFailed(self, err, q):
        logger.error('_responseFailed for %s(%d) -- %s' % (q['clients']['name'], q['clients']['session_id'], err))
        with q['lock']:
            q['timeouts']['delay'].cancel()
            del q['timeouts']['delay']
    def render_GET(self, request):
        return self.render_POST(request)
    def render_POST(self, request):
        #pprint(request)
        #pprint(request.headers)
        #pprint(request.content.getvalue())
        req_data = json.loads(request.content.getvalue())
        request.setHeader('content-type', 'application/json')
        request.setHeader('Access-Control-Allow-Origin','*')
        headers = request.requestHeaders
        ip_chain = headers.getRawHeaders('x-forwarded-for')
        #pprint(req_data)
        logger.debug(req_data)
        
        if req_data.has_key('type'):
            type = req_data['type']
        else:
            req_data['request'] = 'failed'
            req_data['error'] = 'bad_type'
            return json.dumps(req_data)
        if req_data.has_key('args'):
            args = req_data['args']
        else:
            args = {}
            
        if req_data.has_key('username'):
            username = req_data['username']
        else:
            req_data['request'] = 'failed'
            req_data['error'] = 'No username present.'
            logger.error('Rejected request, no username')
            return json.dumps(req_data)

        if req_data.has_key('session_id'):
            session_id = req_data['session_id']
        else:
            req_data['request'] = 'failed'
            req_data['error'] = 'No session_id present.'
            logger.error('Rejected request, no session_id')
            return json.dumps(req_data)
        
        # Catch all
        resp = req_data
        req_data['request'] = 'failed'
        resp['error'] = 'Unknown "type" of command'
        
        if type == 'login':
            resp = self.do_login(username, session_id, req_data['password'], ip_chain)
        elif type == 'get_status':
            if req_data.has_key('last_seen'):
                last_seen = req_data['last_seen']
            else:
                last_seen = 0
            return self.get_status(username, session_id, last_seen, request)
            
        #self._requests.append(request)
        #call = reactor.callLater(10, self._delayedRender, request)
        #request.notifyFinish().addErrback(self._responseFailed, call)
        #return NOT_DONE_YET
        return json.dumps(resp)
    def do_login(self, username, session, password, ip_chain):
        # passwords ignored for now, i.e. all logins are guest logins.        
        # first look for an existing session from same ip address
        q = queue_manager.get_queue(username)
        with q['lock']:
            if session == -1:
                if q['clients']['session_id'] == -1:
                    # New session
                    q['clients']['session_id'] = make_session_id()
                    q['clients']['ip_chain'] = ip_chain
                    q['timeouts']['client'] = reactor.callLater(SESSION_TIMEOUT, self.session_timeout, q)
                    logger.debug('New session %s(%d)' % (q['clients']['name'], q['clients']['session_id']))
                    return {'request':'success', 'session_id':q['clients']['session_id'], 'type':'login'}
                # Old session exists check IP's match
                if q['clients']['ip_chain'] == ip_chain:
                    if q['timeouts'].has_key('client'):
                        q['timeouts']['client'].reset(SESSION_TIMEOUT)
                    else:
                        q['timeouts']['client'] = reactor.callLater(SESSION_TIMEOUT, self.session_timeout, q)
                    logger.debug('Existing Session %s(%d)' % (q['clients']['name'], q['clients']['session_id']))
                    return {'request':'success', 'session_id':q['clients']['session_id'], 'type':'login'}
                logger.error('Reject session IP\'s differ %s(%d)' % (q['clients']['name'], q['clients']['session_id']))
                return { 'request': 'failed', 'error':'User already logged in from different IP', 'type':'login' }
            # We've been given a session id, so possibly old session.
            if q['clients']['session_id'] == -1:
                # looks like session has expired, we'll let them keep there session_id
                q['clients']['session_id'] = session
                q['clients']['ip_chain'] = ip_chain
                if q['timeouts'].has_key('client'):
                    q['timeouts']['client'].reset(SESSION_TIMEOUT)
                else:
                    q['timeouts']['client'] = reactor.callLater(SESSION_TIMEOUT, self.session_timeout, q)
                logger.debug('Refresh expired session %s(%d)' % (q['clients']['name'], q['clients']['session_id']))
                return {'request':'success', 'session_id':q['clients']['session_id'], 'type':'login'}
            if q['clients']['session_id'] == session:
                # Same session, just validate
                q['clients']['ip_chain'] = ip_chain
                if q['timeouts'].has_key('client'):
                    q['timeouts']['client'].reset(SESSION_TIMEOUT)
                else:
                    q['timeouts']['client'] = reactor.callLater(SESSION_TIMEOUT, self.session_timeout, q)
                logger.debug('Existing Session %s(%d)' % (q['clients']['name'], q['clients']['session_id']))
                return {'request':'success', 'session_id':q['clients']['session_id'], 'type':'login'}
            # Sessions don't match, check ips.
            if q['clients']['ip_chain'] == ip_chain:
                if q['timeouts'].has_key('client'):
                    q['timeouts']['client'].reset(SESSION_TIMEOUT)
                else:
                    q['timeouts']['client'] = reactor.callLater(SESSION_TIMEOUT, self.session_timeout, q)
                logger.debug('Existing session (id\'s differ ip\'s same) %s(%d)' % (q['clients']['name'], q['clients']['session_id']))
                return {'request':'success', 'session_id':q['clients']['session_id'], 'type':'login'}
            # IP check failed
            logger.error('Reject session IP\'s differ %s(%d)' % (q['clients']['name'], q['clients']['session_id']))
            return { 'request': 'failed', 'error':'User already logged in from different IP', 'type':'login' }
    def session_timeout(self, q):
        with q['lock']:
            logger.debug('Session timeout %s(%d)' % (q['clients']['name'], q['clients']['session_id']))
            q['clients']['session_id'] = -1
            q['timeouts']['client'] = reactor.callLater(SESSION_REMOVE_TIMEOUT, self.session_remove, q)
    def session_remove(self, q):
        logger.debug('Session remove %s(%d)' % (q['clients']['name'], q['clients']['session_id']))
        queue_manager.remove_queue(q)
    def get_status(self, username, session_id, last_seen, request):
        if not queue_manager.has_key(username):
            logger.debug('Reject get_status -- user not logged in %s(%d)' % (username, session_id))
            return json.dumps({ 'request':'failed', 'error':'No session exists for %s. Please login first.' % username })
        q = queue_manager.get_queue(username)
        if q['clients']['session_id'] != session_id:
            logger.debug('Reject get_status session id\'s differ %s(%d) -- %d' % (q['clients']['name'], q['clients']['session_id'], session_id))
            return json.dumps({ 'request':'failed', 'error':'Non-matching session_id'})
        msgs = q['messages']
        max_msg = 0
        msg_list = []
        for k,v in msgs.iteritems():
            if last_seen < k:
                max_msg = max(max_msg,k)
                msg_list.append(v)
        q['timeouts']['client'].reset(SESSION_TIMEOUT)
        if q['timeouts'].has_key('delay'):
            q['timeouts']['delay'].cancel()
            del q['timeouts']['delay']
        if len(msg_list) == 0:
            # delay till later and try again
            q['requests'].append(request)
            q['timeouts']['delay'] = reactor.callLater(REQUEST_TIMEOUT, self._delayedRender, q)
            request.notifyFinish().addErrback(self._responseFailed, q)
            logger.debug('Deferred get_status response %s(%d)' % (q['clients']['name'], q['clients']['session_id']))
            return NOT_DONE_YET
        # Some messages so just send them.
        logger.debug('get_status returns %d messages %s(%d)' % (len(msg_list), q['clients']['name'], q['clients']['session_id']))
        return json.dumps({ 'request':'success', 'type':'status', 'status_id':max_msg, 'messages':msg_list })
  
def main(options, args):
    if len(args) > 0:
        gsq = GameServerStatusQueryThread(args[0])
        gsq.start()
    else:
        gsq = None
    root = resource.Resource()
    lobby_handler = LobbyHandler(gsq)
    factory = Site(lobby_handler)
    reactor.listenTCP(options.port, factory)
    reactor.run()
    lobby_handler.cleanup()
    if gsq: gsq.abort()

def get_opts():
    usage = 'usage: %prog <options> <game_server:port>'
    parser = OptionParser(usage)
    parser.add_option('-v', '--verbose', action='store_true', default=False, dest='verbose')
    parser.add_option('-p', '--port', action='store', default=8181, dest='port')
    return parser.parse_args()
   
if  __name__ == '__main__':
    # Running standalone mode not as a service.
    options, args = get_opts()
    main(options, args)
else:
    # run under twistd using -y
    options, args = get_opts()
    gsq = GameServerStatusQueryThread(args[0])
    gsq.start()
    root = Resource()
    root.putChild('', FormPage())
    application = Application('Lobby Service')
    TCPServer(options.port, Site(root)).setServiceParent(application)
