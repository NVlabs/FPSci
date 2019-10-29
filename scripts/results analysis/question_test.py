import sys
from FPSci_Importer.Importer import Importer

# Get the database
if len(sys.argv) < 2: 
    raise Exception("Provide input db as argument!")
db = Importer(sys.argv[1])

questions = db.getQuestionResponses()

for q in questions: 
    print("{0} : {1}".format(q.question, q.response))