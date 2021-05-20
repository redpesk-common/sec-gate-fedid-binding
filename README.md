# Fedid Binding

sec-gate-fedid-binding exposes through a standard set of REST/Websocket APIs a simple mechanism to launch within secure sandbox containers your preferred Linux native commands or scripts: bash, python, node, ruby ...
Sandbox security model scales from simple Linux access control to advanced mechanism as capabilities, cgroups, namespaces, ...

**Developer and user documentation at [redpesk-docs](http://docs.redpesk.bzh/docs/en/master/apis-services/sec-gate-fedid-binding/fedid_binding_doc.html)**

## Based on AGL/AFB-V3 micro-service architecture

sec-gate-fedid-binding can be used in standalone to provide a simple secure HTML5 interface to your preferred scripts. When used in conjunction with AGL/AFB framework its also leverage SeLinux/Smack to check API access with Cynara.

![fedid-biding-html5](docs/assets/sec-gate-fedid-binding-dirconf.jpg)


Fulup notes:
 * si double click sur login alors on popup pour logout ou anulation
 
 * si federate on ajoute un link de fedsocial de plus

 * prevoir le parsing des roles dans les idp
 * ajouter la notion de label de securité

 * prevoir une double authentification (oidc + badge)

 * ajouter un timeout sur les session par profil 