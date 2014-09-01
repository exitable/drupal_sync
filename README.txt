Description
-----------
The present module allows synchronizing content among multiple websites.
Supported entity types: node and taxomony, menu items.
Many hooks were used during creation of the module. It allows widening
synchronization functional by developing additional modules.
Later, it is planned to implement an opportunity of users synchronization
(Entity type: user) by creation of a submodule with the help of main module
hooks. This way we will create an example of main module functional widening
opportunity.
At the current version we implemented an interface of websites interaction
through XML-RPC, i.e.authorization, settings sending/getting , content
sending/getting (entity fields), getting files from remote website.

All synchronization settings can be indicated on one website and all the other
 remote websites will get them automatically (use checkbox "Set the same field
mapping on the remote site") )

## Settings

1) These actions should be taken on each website that participate in synchronization process.
On the page of module settings it is needed :
- to specify update frequency, number of entities to be updated per 1 request,
- to choose entities available for synchronization (node types, taxonomy vocabularies) ,

Also in "Remote sites" section it is needed to specify Name, URL, Login and password for each remote site.

2) After, visit one of the sites (it is desirable to choose a major site connected with the number of secondary sites).
Continue synchronization setting on Entity Edit page (i.e. news node).
On "Syncronization settings" layout there are all deleted sites that are indicated in main module settings.
For each remote site you can choose entities (of the same type as current local ones) that are allowed to be syncronized on this site . So in this case you can choose in "select " only allowed remote entities of "node" type . After choosing node type, local and remote fields are loaded. Then you should choose equivalence of remote and local fields.

If you choose "Set field equivalence on the remote site" during saving your settings on local site, all settings will be saved on remote site as well. This eases equivalence setting on all sites.
