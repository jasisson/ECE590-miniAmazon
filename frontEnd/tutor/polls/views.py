from django.shortcuts import get_object_or_404, render
from django.http import HttpResponseRedirect, HttpResponse
from django.template import loader
from django.urls import reverse
from django.views import generic

from .models import Choice, Question, User, Catalog, Orders

class IndexView(generic.ListView):
        template_name = 'polls/index.html'
        context_object_name = 'latest_question_list'

        def get_queryset(self):
                """Return the last five published questions."""
                return Question.objects.order_by('-pub_date')[:5]

class DetailView(generic.DetailView):
        model = Question
        template_name = 'polls/detail.html'
        
class ResultsView(generic.DetailView):
        model = Question
        template_name = 'polls/results.html'

def catView(request, user_id): #add userID for nicety?
        if request.method == "GET":
                c_list = Catalog.objects.all();
                return render (request, 'polls/catalog.html', {
                        'c_list':c_list,
                        'user_id': user_id,
                })
        else:
                c_list = Catalog.objects.order_by('cat_type_num');
                return render (request, 'polls/catalog.html', {
                        'c_list':c_list,
                        'user_id': user_id,
                })

def orderView(request, user_id): #add userID for nicety?
        if request.method == "GET":
                o_list = Orders.objects.all();
                return render (request, 'polls/orders.html', {
                        'o_list':o_list,
                        'user_id': user_id,
                })
        else:
                return render (request, 'polls/orders.html', {
                        'o_list':o_list,
                        'user_id': user_id,
                })

def createBuyView(request, user_id):
        print("called createBuy")
        cat_pid = request.POST['item']
        desc = request.POST['desc']
        buy_order =    Orders(order_usrID=user_id, order_desc = desc, order_pid = cat_pid, order_status = 0, order_quant = request.POST['quant' + cat_pid], order_adrX = request.POST['adrX'+cat_pid], order_adrY = request.POST['adrY'+cat_pid])
        buy_order.save()
        return HttpResponseRedirect(reverse('polls:catalog', args=(user_id)))

def LoginView(request):
    return render(request, 'polls/login.html')

def LoginHandleView(request):
    if ('username' in request.POST and (len(request.POST['username']) > 0)):
        if ('buttonlogin' in request.POST and User.objects.filter(u_name = request.POST['username'])):
            user = get_object_or_404(User, u_name=request.POST['username'])
            return HttpResponseRedirect(reverse('polls:catalog', args=(user.id, )))
        if ('buttonsign' in request.POST and (not User.objects.filter(u_name = request.POST['username'])) and 'useremail' in request.POST and (len(request.POST['useremail']) > 0)):
            n_user = User(u_email=request.POST['useremail'], u_name=request.POST['username'])
            n_user.save()
    return HttpResponseRedirect(reverse('polls:login', args=()))

def EventView(request, user_id):
    event_list = Event.objects.filter(pee_set__u_id=user_id).order_by('-e_time')
    return render(request, 'polls/event.html', {
                  'event_list': event_list,
                  'user_id': user_id,
                  'user_name': get_object_or_404(User, pk=user_id).u_name,
                  'pe_list': get_object_or_404(User, pk=user_id).peu_set.all(),
                  })




def vote(request, question_id):
        question = get_object_or_404(Question, pk=question_id)
        try:
                selected_choice = question.choice_set.get(pk=request.POST['choice'])
        except (KeyError, Choice.DoesNotExist):
                # Redisplay the question voting form.
                return render(request, 'polls/detail.html', {
                        'question': question,
                        'error_message': "You didn't select a choice.",
                })
        else:
                selected_choice.votes += 1
                selected_choice.save()
                # Always return an HttpResponseRedirect after successfully dealing
                # with POST data. This prevents data from being posted twice if a
                # user hits the Back button.
                return HttpResponseRedirect(reverse('polls:results', args=(question.id,)))
